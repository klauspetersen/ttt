//
// Created by klauspetersen on 2/14/16.
//

#ifndef TTT_SALEAE_H
#define TTT_SALEAE_H

class ResourceItf{
public:
    virtual ~ResourceItf(){};
    virtual void open() = 0;
    virtual void close() = 0;
    virtual void read() = 0;
    virtual void *data() = 0;
};

class Saleae: public virtual ResourceItf{
    void open(){};
    void close(){};
    void read(){};
    void *data(){ return nullptr;};
};


#endif //TTT_SALEAE_H
