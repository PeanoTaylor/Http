#pragma once
#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include <string>
#include <memory>

using std::string;
using AmqpClient::Channel;
using AmqpClient::BasicMessage;
using AmqpClient::Envelope;


class MqClientWrapper
{
public:
    MqClientWrapper(const string &host,int port,const string &username,const string &password,const string &vhost);

    ~MqClientWrapper();

    // 声明交换机和队列
    void declare(const string& exchange,const string& queue,const string& routing_key,const string& type = Channel::EXCHANGE_TYPE_DIRECT);

    // 发送消息
    void publish(const string& exchange,const string& routing_key,const string& body);

    // 消费消息（阻塞模式）
    bool consume(const string& queue, Envelope::ptr_t& env, int timeout_ms = 1000);

    // 确认消息
    void ack(const Envelope::ptr_t& env);

private:
    Channel::ptr_t m_channel;
};

