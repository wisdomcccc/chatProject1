#include "chat_message.h"
#include <boost/asio.hpp>
#include <deque>
#include <iostream>
#include <set>
#include <utility>
#include <cstdlib>
#include <queue>
using boost::asio::ip::tcp;

class chat_participant
{
public:
	virtual ~chat_participant(){}
	virtual void deliver(const chat_message& msg) = 0;
};
using chat_participant_ptr = std::shared_ptr<chat_participant>;

#define max_size 100
class chat_room
{
public:
	void join(chat_participant_ptr p){
		participant_set.insert(p);
		for(const auto& msg : msg_deque){
			p->deliver(msg);
		}
	}
	void receive(const chat_message msg){
		deliver(msg);
		msg_deque.push_back(msg);
		if(msg_deque.size() > max_size)
			msg_deque.pop_front();
	}
	void deliver(const chat_message& msg){
		for(const auto& participant : participant_set){
			participant->deliver(msg);
		}
	}
	void leave(chat_participant_ptr p){
		participant_set.erase(p);
	}
private:
	std::deque<chat_message> msg_deque;
	std::set<chat_participant_ptr> participant_set;
};

class session : public chat_participant,public std::enable_shared_from_this<session>
{
public:
	session(chat_room& rom,tcp::socket& sock):chatroom(rom),mysocket(std::move(sock)),membername("游客"){}
	void deliver(const chat_message& msg){
		bool on_write = !write_queue.empty();
		write_queue.push(msg);
		if(!on_write)
			do_write();	
	}
	void start(){
		do_read_header();	
	}
private:
	void do_write(){
		auto self(shared_from_this());
		boost::asio::async_write(mysocket,boost::asio::buffer(write_queue.front().data(),
																													write_queue.front().length()),
			[this,self](auto err,std::size_t	){
				if(!err){
					write_queue.pop();
					if(!write_queue.empty())
						do_write();
				}
				else{
					chatroom.leave(shared_from_this());
				}
			});
	}
	void do_read_header(){
		auto self(shared_from_this());
		boost::asio::async_read(mysocket,boost::asio::buffer(read_msg.data(),chat_message::header_length),
			[this,self](auto err,std::size_t){
				std::cout<<"准备解码\n";
				if(!err && read_msg.decode_header()){
					std::cout<<"准备读取\n";
					do_read_body();
				}
				else{
					std::cout<<"解码失败\n";
					chatroom.leave(shared_from_this());
				}
			});
	}
	template<typename T>
	T toObj(){
		T obj;
		std::stringstream ss(std::string(read_msg.body(),read_msg.body()+read_msg.get_body_length()));
		boost::archive::text_iarchive ia(ss);
		ia & obj;
		return obj;
	}
  void handleMessage(){
		using ChatType = chat_header::chat_type;
		ChatType type = read_msg.getChatType();
		if(type==ChatType::BINDNAME){
			membername = toObj<SBindName>();
		}
		else if(type== ChatType::CHATINFO){
			SChatInfo chat = toObj<SChatInfo>();
			SRoomInfo roominfo(membername,std::move(chat));
			std::string seriliation = serialize(roominfo);
			read_msg.setMessage(ChatType::ROOMINFO,seriliation);
			chatroom.receive(read_msg);
		}
		else {}
	}
	void do_read_body(){
		auto self(shared_from_this());
		boost::asio::async_read(mysocket,boost::asio::buffer(read_msg.body(),read_msg.get_body_length()),
			[this,self](auto err,std::size_t){
				if(!err){
					handleMessage();
					std::cout<<"接收主体\n";
					do_read_header();
				}
				else{
					chatroom.leave(shared_from_this());
				}
			});
	}		
	chat_room& chatroom;
	tcp::socket mysocket;
	SBindName membername;
	std::queue<chat_message> write_queue;
	chat_message read_msg;
};

class server
{
public:
	server(boost::asio::io_service& io)
	:chatroom(),_acceptor(io,tcp::endpoint(tcp::v4(),5051)),_socket(io){
			
	}
	void start(){
		accept();
	}
	void accept(){
		_acceptor.async_accept(_socket,
			[this](const auto& err){
				if(!err){
					auto pointer = std::make_shared<session>(chatroom,_socket);
					chatroom.join(pointer);	
					pointer->start();				
				}
				accept();
			});
	}
private:
	chat_room chatroom;
	tcp::acceptor _acceptor;
	tcp::socket _socket;
};
int main(){
	boost::asio::io_service io;
	server s(io);
	s.start();
	io.run();
}

