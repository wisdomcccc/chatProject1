#pragma once
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <string>
#include <iostream>
class SBindName
{
private:
	friend class boost::serialization::access;
	template <class Archive>
	void serialize(Archive& ar,const unsigned int version){
		ar & m_bindName;
	}
	std::string m_bindName;
public:
	explicit SBindName(std::string& name):m_bindName(name){}
	explicit SBindName(std::string&& name):m_bindName(std::move(name)){}
	SBindName(const SBindName& _bindname):m_bindName(_bindname.m_bindName){}
	SBindName(){}
	const std::string& bindName(){ return m_bindName;}
};

class SChatInfo
{
private:
	friend class boost::serialization::access;
	template <class Archive>
	void serialize(Archive& ar,const unsigned int version){
		ar & m_chatInfo;
	}
	std::string m_chatInfo;
public:
	explicit SChatInfo(std::string& info) : m_chatInfo(info){}
	explicit SChatInfo(std::string&& info) :m_chatInfo(std::move(info)){}
	SChatInfo(const SChatInfo& right):m_chatInfo(right.m_chatInfo){}
	SChatInfo(SChatInfo&& right):m_chatInfo(std::move(right.m_chatInfo)){}
	SChatInfo(){}
	const std::string& chatInfo(){ return m_chatInfo;}
};

class SRoomInfo
{
private:
	friend class boost::serialization::access;
	template <class Archive>
	void serialize(Archive& ar,const unsigned int version){
		ar & m_bind;
		ar & m_chat;
	}
	SBindName m_bind;
	SChatInfo m_chat;
public:
	SRoomInfo(std::string name,std::string info):m_bind(std::move(name)),m_chat(std::move(info)){}
	SRoomInfo(SBindName& bindname,SChatInfo&& chatinfo):m_bind(bindname),m_chat(std::move(chatinfo)){}
	SRoomInfo(){}
	const std::string& name(){return m_bind.bindName();}
	const std::string& chat(){return m_chat.chatInfo();}
	void show(){
		std::cout<< m_bind.bindName() << " say\n" << m_chat.chatInfo() << std::endl;
	}
};
