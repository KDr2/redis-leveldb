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
        HASH = 5,
    };
};

inline string _encode_compdata_size_key(const string &name, CompDataType::COM_DATA_TYPE t){
    string ret;
    ret.append(1, (char)CompDataType::SIZE);
    ret.append(1, (char)t);
    ret.append("size", 4);
    ret.append(1, (uint8_t)name.size());
    ret.append(name.data(), name.size());
    return ret;
}

inline string _encode_compdata_key(const string &cname, const string &memname, CompDataType::COM_DATA_TYPE t){
    string ret;
    ret.append(1, (char)t);
    ret.append(1, (uint8_t)cname.size());
    ret.append(cname.data(), cname.size());
    if(memname.size()<1){
        return ret;
    }
    ret.append(1, (uint8_t)memname.size());
    ret.append(memname.data(), memname.size());
    return ret;
}

inline string _encode_set_key(const string &setname, const string &memname){
    return _encode_compdata_key(setname, memname, CompDataType::SET);
}

inline string _encode_hash_key(const string &hname, const string &memname){
    return _encode_compdata_key(hname, memname, CompDataType::HASH);
}

inline string _decode_key(const string key){
    uint8_t com_size = (uint8_t)key[1] +3;
    if(key.size() < com_size) return "";
    return key.substr(com_size);
}
