#ifndef SOURCE_H
#define SOURCE_H


class Source
{
public:
    virtual ~Source() = default;
    virtual void update() = 0;
};

#endif // SOURCE_H