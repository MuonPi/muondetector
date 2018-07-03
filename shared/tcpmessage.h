#ifndef TCPMESSAGE_H
#define TCPMESSAGE_H

template <typename T> class TcpMessage
{
public:
    TcpMessage() : value(){}
    TcpMessage(const T val);
    const T& operator()(){
        return value;
    }

private:
    T value;
};

#endif // TCPMESSAGE_H
