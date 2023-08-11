#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "chat_body.h"

template<typename T>
std::string serialize(const T& obj){
	std::stringstream ss;
	boost::archive::text_oarchive oa(ss);
	oa & obj;
	return ss.str();
}
class chat_header
{
public:
	enum chat_type{
		BINDNAME=1,
		CHATINFO=2,
		ROOMINFO=3	
	};
	chat_type getType()const{ return body_type;}
	void setType(chat_type& type){ body_type = type;}
	chat_header(chat_type type):body_type(type) {}
	chat_header():body_type(CHATINFO),body_size(0){}
	void setBodySize(size_t size){ body_size=size;}
	size_t getBodySize()const {return body_size;}
private:
	chat_type body_type;
	size_t body_size;
};

class chat_message
{
public:
	enum {header_length = sizeof(chat_header)};
	enum {max_body_size = 256};
	enum {msg_size = sizeof(chat_header)+ max_body_size};
	chat_message(){}

	const char* data() const {return data_;}
	char* data() {return data_;}

	char* body() {return data_ + header_length;}
	const char* body()const {return data_ + header_length;}
	chat_header::chat_type getChatType() const {return header.getType();}
	size_t get_body_length() const {return header.getBodySize();}
	size_t length() const {return header_length + get_body_length();}
	void setMessage(chat_header::chat_type type,const void* buffer,size_t bufferSize){
		header.setType(type);
		header.setBodySize(bufferSize);	
		memcpy(data_,&header,header_length);
		memcpy(body(),buffer,bufferSize);
	}
	void setMessage(chat_header::chat_type type,std::string& buffer)
	{
		setMessage(type,buffer.data(),buffer.size());
	}
	bool parseMessage(const std::string& input){
		auto pos = input.find_first_of(" ");
		if(pos==std::string::npos)
			return false;
		if(pos==0)
			return false;
		auto command = input.substr(0,pos);
		if(command == "BindName"){
			std::string name = input.substr(pos+1);
			std::string output = serialize(SBindName(std::move(name)));
			setMessage(chat_header::chat_type::BINDNAME,output);
			return true;
		}
		else if(command == "Chat"){
			std::string chat = input.substr(pos+1);
			std::string output = serialize(SChatInfo(std::move(chat)));
			setMessage(chat_header::chat_type::CHATINFO,output);
			return true;
		}
		else return false;
	}
	bool decode_header(){
		memcpy(&header,data_,header_length);	
		return true;
	}
private:
	chat_header header;
	char data_[msg_size];
};





