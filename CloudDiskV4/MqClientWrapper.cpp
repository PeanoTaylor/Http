#include "MqClientWrapper.hpp"
using namespace AmqpClient;

MqClientWrapper::MqClientWrapper(const string &host, int port, const string &username, const string &password, const string &vhost)
{
    m_channel = Channel::Create("127.0.0.1", 5672, "guest", "guest", "/");
}

MqClientWrapper::~MqClientWrapper() {
    // 如果后续需要释放资源可以写在这里，目前空实现即可
}

// 声明交换机和队列
void MqClientWrapper::declare(const string &exchange, const string &queue, const string &routing_key, const string &type)
{
    m_channel->DeclareExchange(exchange, type, true);          // durable exchange
    m_channel->DeclareQueue(queue, true, false, false, false); // durable queue
    m_channel->BindQueue(queue, exchange, routing_key);
}

// 发送消息
void MqClientWrapper::publish(const string &exchange, const string &routing_key, const string &body)
{
    auto msg = BasicMessage::Create(body);
    msg->DeliveryMode(BasicMessage::dm_persistent); // durable
    m_channel->BasicPublish(exchange, routing_key, msg);
}

// 消费消息（阻塞模式）
bool MqClientWrapper::consume(const string &queue, Envelope::ptr_t &env, int timeout_ms)
{
    static string consumer_tag;
    if (consumer_tag.empty())
    {
        consumer_tag = m_channel->BasicConsume(queue, "", true, false, false);
    }
    return m_channel->BasicConsumeMessage(consumer_tag, env, timeout_ms);
}

// 确认消息
void MqClientWrapper::ack(const Envelope::ptr_t &env)
{
    m_channel->BasicAck(env);
}
