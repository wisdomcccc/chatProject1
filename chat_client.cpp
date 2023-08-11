#include "chat_message.h"
#include <boost/asio.hpp>
#include <queue>
#include <iostream>
#include <thread>
#include <cstdlib>
#include <cstring>
using boost::asio::ip::tcp;
class chat_client
{
public:
	chat_client(boost::asio::io_service& io,tcp::resolver::iterator endpoint_iterator)
	:io(io),mysocket(io),connect_iterator(endpoint_iterator){}
	void connect(){
		boost::asio::async_connect(mysocket,connect_iterator,
			[this](auto ec,tcp::resolver::iterator){
				if(!ec){
					do_read_header();
					std::cout<<"连接成功\n";
				}
			});
	}
	void do_read_header(){
		boost::asio::async_read(mysocket,boost::asio::buffer(read_msg.data(),chat_message::header_length),
			[this](auto ec,size_t){
				if(!ec){
					read_msg.decode_header();
					do_read_body();
				}
				else{
					mysocket.close();
				}
			});
	}
	template<typename T>
	T toObj(){
		T obj;
		std::stringstream ss(std::string(read_msg.body(),read_msg.body()+ read_msg.get_body_length()));
		boost::archive::text_iarchive ia(ss);
		ia & obj;
		return obj;
	}
	void do_read_body(){
		boost::asio::async_read(mysocket,boost::asio::buffer(read_msg.body(),read_msg.get_body_length()),
			[this](auto ec,size_t){
				if(!ec){	
					SRoomInfo chat = toObj<SRoomInfo>();
					chat.show();
					do_read_header();
				}
				else{
					mysocket.close();
				}
			});
	}
	void write(const chat_message& msg){
		io.post([this,msg](){
			bool on_write = !write_msg_queue.empty();
			write_msg_queue.push(msg);
			if(!on_write)
				do_write();
		});
	}
	void do_write(){
		auto& msg = write_msg_queue.front();
		boost::asio::async_write(mysocket,boost::asio::buffer(msg.data(),msg.length()),
			[this](auto ec,size_t){
				if(!ec){
					std::cout<<"发送成功\n";
					write_msg_queue.pop();
					if(!write_msg_queue.empty()){
						do_write();
					}
				}
				else
					mysocket.close();
			});
	}
	void close(){
		io.post([this](){mysocket.close();});
	}
private:
	boost::asio::io_service& io;
	tcp::socket mysocket;
	tcp::resolver::iterator connect_iterator;
	chat_message read_msg;
	std::queue<chat_message> write_msg_queue;
};

int main(){
	boost::asio::io_service io;
	tcp::resolver resolver(io);
	auto endpoint_iterator = resolver.resolve({"localhost","5051"});
	chat_client client(io,endpoint_iterator);
	client.connect();
	char wbuf[chat_message::max_body_size + 1];
	std::thread t([&io](){io.run();});
	while(std::cin.getline(wbuf,chat_message::max_body_size + 1)){
		chat_message msg;
		std::string input(wbuf,wbuf + strlen(wbuf));
		if(msg.parseMessage(input)){
			client.write(msg);
		}
	}

	client.close();
	t.join();
}
