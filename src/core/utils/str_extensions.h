#pragma once
/*
 * Developed by Guillaume Gomez <guillaume.gomez@gmail.com>.
 * This header file has been created to make operations on string easier.
 *
 * List of functions:
 * - getValueFromString
 * - toString
 * - split
 * - join
 * - getBetween
 * - getListOfItem
 * - replace
 * - numberOfOccurence
 * - truncate
 */


#ifndef __STRING_UTILS__
#define __STRING_UTILS__

#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <cstring>
#include <regex>

namespace string_utils
{

    inline bool isInt(const std::string& val) { 
        std::regex r_int =
            std::regex("[\\+-]?((\\d+(\\.(0*))?)(e[\\+]?\\d+)?)", std::regex::icase);
        
        return std::regex_match(val, r_int); 
    }

    /*
     * Checks if `val` is any kind of float.
     */
    inline bool isFloat(const std::string& val) { 
        std::regex r_float = std::regex(
            "[\\+-]?(((\\d*\\.\\d+)|(\\d+(\\.(0*))?))(e[\\+-]?((\\d*\\.\\d+)|(\\d+(\\.(0*))?)))?)",
            std::regex::icase);

        return std::regex_match(val, r_float); 
    }


    // get a value from a string, for example :
    // getValueFromString<int>("42") -> 42
    template<typename T>
    inline T getValueFromString(std::string const& s)
    {
        T           var(0);
        std::istringstream  iss;

        if (s != "")
        {
            iss.str(s);
            iss >> var;
        }
        return var;
    }

    template<>
    inline std::string    getValueFromString<std::string>(std::string const& s)
    {
        std::string tmp;

        tmp = s;
        return tmp;
    }

    template<typename T>
    inline T getValueFromString(const char* s)
    {
        std::string str;

        if (s)
            str = s;
        return getValueFromString<T>(str);
    }

    // convert a value into a string, for example :
    // toString<int>(42) -> "42"
    // toString<float>(42.4f) -> "42.4f"
    template<typename T>
    std::string   toString(T const& var)
    {
        std::ostringstream  oss;
        std::string     s;

        oss << var;
        s = oss.str();
        return s;
    }

    // splits the string into substrings wherever key occurs, example :
    // split<std::string>("salut les   amis", " ") -> std::vector<std::string>{"salut", "les", "amis"}
    // if you want to keep empty parts, just set the last argument to true, example :
    // split<std::string>("salut les   amis", " ", true) -> std::vector<std::string>{"salut", "les", "", "", "amis"}
    template<typename T, typename U>
    std::vector<U>  split(T const& value, const char* key, bool keepEmptyPart = false)
    {
        std::vector<U>  result;
        size_t          pos;
        std::string     sub;
        int             size;
        std::string     copy(toString<T>(value));

        if (!key)
            return result;
        size = strlen(key);
        while ((pos = copy.find(key)) != std::string::npos)
        {
            sub = copy.substr(0, pos);
            copy.erase(0, pos + size);
            if (!sub.empty() || keepEmptyPart)
                result.push_back(getValueFromString<U>(sub));
        }
        if (copy != "")
            result.push_back(getValueFromString<U>(copy));
        return result;
    }

    template<typename T, typename U>
    std::vector<U>  split(T const& value, std::string const& key, bool keepEmptyPart = false)
    {
        return split<T, U>(value, key.c_str(), keepEmptyPart);
    }

    // join every elements of the vector in a single string,
    // separated by the given separator. For example :
    // join<std::string>(std::vector<std::string>{"salut", "les", "amis"}, "/") ->
    // "salut/les/amis"
    template<typename T>
    std::string join(std::vector<T> const& c, const char* separator)
    {
        std::ostringstream  os;
        unsigned int    i(0);
        std::string     str;

        while (i < c.size())
        {
            os.str("");
            os.clear();
            os << c[i++];
            str += os.str();
            if (i < c.size() && separator)
                str += separator;
        }
        return str;
    }

    template<typename T>
    std::string join(std::vector<T> const& c, std::string const& separator)
    {
        return join<T>(c, separator.c_str());
    }

    // return the value between the two given keys, example :
    // getBetween<int>("et42tralala", "et", "tralala") -> 42
    template<typename T>
    T getBetween(std::string const& c, const char* key1, const char* key2)
    {
        size_t      pos1, pos2;

        if (!key1 || !key2 ||
            (pos1 = c.find(key1)) == std::string::npos ||
            (pos2 = c.find(key2)) == std::string::npos)
            return getValueFromString<T>("");
        pos1 += strlen(key1);
        if (pos1 >= pos2)
            return getValueFromString<T>("");
        return getValueFromString<T>(c.substr(pos1, pos2 - pos1).c_str());
    }

    template<typename T>
    T getBetween(const char* st, const char* key1, const char* key2)
    {
        std::string str;

        if (st)
            str = st;
        return getBetween<T>(str, key1, key2);
    }

    // return a vector with every values between the two keys, example :
    // getListOfItem<int>("<a>42</a><b><s>tralala</s><s>bis</s><a>39</a></b>") ->
    // std::vector<int>{42, 39}
    template<typename T>
    std::vector<T>  getListOfItem(std::string st, const char* key1, const char* key2)
    {
        std::vector<T>  vec;
        std::string     str;
        size_t          find;
        int             length(key2 ? strlen(key2) : 0);

        str = getBetween<T>(st, key1, key2);
        for (; !str.empty(); str = getBetween<T>(st, key1, key2))
        {
            vec.push_back(str);
            if ((find = st.find(key1)) == std::string::npos)
                return vec;
            st.erase(find, str.size() + length);
        }
        return vec;
    }

    template<typename T>
    std::vector<T>  getListOfItem(const char* st, const char* key1, const char* key2)
    {
        std::string s;

        if (st)
            s = st;
        return getListOfItem<T>(s, key1, key2);
    }

    // replace every occurences of the specified substring (toReplace) by another one (replacement)
    // in the given argument (s), example :
    // replace<std::string>("salut les d'jeuns !", " ", "/") -> "salut/les/d'jeuns/!"
    template<typename T>
    std::string replace(T const& s, const char* toReplace, const char* replacement)
    {
        std::vector<T>  vec;

        if (!toReplace || !replacement)
            return toString<T>(s);
        vec = split<T, T>(s, toReplace);
        return join<T>(vec, replacement);
    }

    template<typename T>
    std::string replace(T const& s, std::string const& toReplace,
        std::string const& replacement)
    {
        return replace<T>(s, toReplace.c_str(), replacement.c_str());
    }

    template<typename T>
    std::string replace(T const& s, const char* toReplace,
        std::string const& replacement)
    {
        return replace<T>(s, toReplace, replacement.c_str());
    }

    template<typename T>
    std::string replace(T const& s, std::string const& toReplace,
        const char* replacement)
    {
        return replace<T>(s, toReplace.c_str(), replacement);
    }

    // return the number of occurences of the given argument (toFind) in the given string (s)
    // example : numberOfOccurence("bonjour les gens", " ") -> 2
    template<typename T>
    unsigned int  numberOfOccurence(std::string const& s, T toFind)
    {
        std::string   tmp(toString<T>(toFind)), tmp2(s);
        unsigned int  found(0);
        size_t        pos(0);

        if (s == "" || toFind == "")
            return 0;
        while ((pos = tmp2.find(tmp, 0)) != std::string::npos) {
            ++found;
            tmp2.erase(0, pos + 1);
        }
        return found;
    }

    template<typename T>
    unsigned int  numberOfOccurence(char const* s, T toFind)
    {
        std::string tmp(s ? s : "");

        if (tmp == "")
            return 0;
        return numberOfOccurence<T>(tmp, toFind);
    }

    // truncate the given parameter at the given position
    template<typename T>
    T truncate(T& s, unsigned int position)
    {
        std::string tmp = toString<T>(s);

        if (position < tmp.length())
            tmp.resize(position);
        return getValueFromString<T>(tmp);
    }
}

#endif