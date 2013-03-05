/*-*- c++ -*-
 *
 * rl_compdata.h
 * author : KDr2
 *
 */

#include <string>

using std::string;

struct CompDataType{
    enum COM_DATA_TYPE{
        NORMAL = 1, // for k-v, not in use now
        SIZE = 2,
        SET = 3,
        ZSET = 4,
    };
};

inline string _encode_compdata_size_key(const string &name){
    string ret;
    ret.append(1, (char)CompDataType::SIZE);
    ret.append("size", 4);
    ret.append(1, (uint8_t)name.size());
    ret.append(name.data(), name.size());
    return ret;
}

inline string _encode_set_kv_key(const string &setname, const string &memname){
    string ret;
    ret.append(1, (char)CompDataType::SET);
    ret.append(1, (uint8_t)setname.size());
    ret.append(setname.data(), setname.size());
    if(memname.size()<1){
        return ret;
    }
    ret.append(1, (uint8_t)memname.size());
    ret.append(memname.data(), memname.size());
    return ret;
}

inline string _decode_key(const string key){
    uint8_t com_size = (uint8_t)key[1] +3;
    if(key.size() < com_size) return "";
    return key.substr(com_size);
}
