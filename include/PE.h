#ifndef PE_H
#define PE_H

class PE {
public:
    PE(int src, int qos);

    int getSrcId() const;
    int getQoS() const;

private:
    int src_id;
    int qos_value;
};

#endif // PE_H
