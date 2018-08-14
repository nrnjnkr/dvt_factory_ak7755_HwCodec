/*
 * LuaTables++
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


#ifndef LUATABLES_H
#define LUATABLES_H

#include <iostream>
#include <assert.h>
#include <cstdlib>
#include <string>
#include <vector>
#include <atomic>

struct LuaKey
{
    enum Type
    {
      String,
      Integer
    };

    Type type;
    int int_value;
    std::string string_value;
    std::string to_string()
    {
      std::string str;
      switch(type) {
        case LuaKey::Integer: {
          str = std::to_string(int_value);
          str += std::string(" (int");
        }break;
        case LuaKey::String: {
          str = string_value + std::string(" (string)");
        }break;
      }
      return str;
    }

    bool operator<(const LuaKey& rhs) const
    {
      if (type == String && rhs.type == Integer) {
        return false;
      } else if (type == Integer && rhs.type == String) {
        return true;
      } else if (type == Integer && rhs.type == Integer) {
        return int_value < rhs.int_value;
      }

      return string_value < rhs.string_value;
    }

    LuaKey(const char* key_value) :
        type(String),
        int_value(0),
        string_value(key_value)
    {}
    LuaKey(const std::string& key_value) :
      type(String),
      int_value(0),
      string_value(key_value)
    {}
    LuaKey(int key_value) :
        type(Integer),
        int_value(key_value),
        string_value("")
    {}
};

inline std::ostream& operator<<(std::ostream& output, const LuaKey &key)
{
  if (key.type == LuaKey::Integer) {
    output << key.int_value << "(int)";
  } else {
    output << key.string_value << "(string)";
  }
  return output;
}

/// Reference counting Lua state
struct lua_State;
class LuaTable;
class LuaTableNode;
class LuaStateRef
{
    friend class LuaTable;
    friend class LuaTableNode;
  public:
    LuaStateRef() :
        L(NULL),
        count(0),
        freeOnZeroRefs(true)
    {
    }

    LuaStateRef* acquire()
    {
      ++ count;
      return this;
    }

    int release()
    {
      -- count;
      return count;
    }

  protected:
    lua_State       *L;
    std::atomic_int  count;
    bool             freeOnZeroRefs;
};

class LuaTable
{
    friend class LuaTableNode;
  public:
    LuaTable() :
        filename(""),
        luaStateRef(NULL),
        L(NULL),
        luaRef(-1),
        referencesGlobal(false)
    {
    }
    LuaTable(const LuaTable &other);
    virtual ~LuaTable();

    LuaTable& operator=(const LuaTable &other);
    LuaTableNode operator[](const char* key);
    LuaTableNode operator[](const std::string& key);
    LuaTableNode operator[](int key);

    int length();
    void addSearchPath(const char* path);
    std::string serialize();

    /// Serializes the data in a predictable ordering.
    std::string orderedSerialize();

    /// Pushes the Lua table onto the stack of the internal Lua state.
    //  I.e. makes the Lua table active that is associated with this
    //  LuaTable.
    void pushRef();
    /// Pops the Lua table from the stack of the internal Lua state.
    //  Cleans up a previous pushRef()
    void popRef();

    static LuaTable fromFile(const char *_filename);
    static LuaTable fromLuaExpression(const char* lua_expr);
    static LuaTable fromLuaState(lua_State *L);

  protected:
    static std::string get_file_directory(const char *filename);
    static std::string get_file_basename(const char *filename);
    static std::string file_name_to_table_name(const std::string& filename);

  protected:
    std::string  filename;
    LuaStateRef *luaStateRef;
    lua_State   *L;
    int          luaRef;
    bool         referencesGlobal;
};

class LuaTableNode
{
    friend class LuaTable;
  public:
    LuaTableNode() :
        parent(NULL),
        luaTable(NULL),
        key(""),
        stackTop(0)
    {
    }
    virtual ~LuaTableNode(){}

    LuaTableNode operator[](const char *child_str);
    LuaTableNode operator[](const std::string &child_str);
    LuaTableNode operator[](int child_index);

    bool stackQueryValue();
    void stackPushKey();
    void stackCreateValue();
    void stackRestore();
    LuaTable stackQueryTable();
    LuaTable stackCreateLuaTable();

    std::vector<LuaKey> getKeyStack();
    std::string keyStackToString();

    bool exists();
    void remove();
    size_t length();
    std::vector<LuaKey> keys();

    // Templates for setters and getters. Can be specialized for custom types.
    template<typename T>
    void set(const T &value);

    template<typename T>
    T getDefault(const T &default_value);

    template<typename T>
    T get()
    {
      if (!exists()) {
        ERROR("Error: cannot find value %s.", keyStackToString().c_str());
      }
      return getDefault(T());
    }
    template<typename T>
    T get(const T& def_value)
    {
      if (!exists()) {
        ERROR("Error: cannot find value %s.", keyStackToString().c_str());
      }
      return getDefault(def_value);
    }

    // convenience operators (assignment, conversion, comparison)
    template<typename T>
    void operator=(const T &value)
    {
      set<T>(value);
    }
    template<typename T>
    operator T()
    {
      return get<T>();
    }
    template<typename T>
    bool operator==(T value)
    {
      return value == get<T>();
    }
    template<typename T>
    bool operator!=(T value)
    {
      return value != get<T>();
    }

  protected:
    LuaTableNode *parent;
    LuaTable     *luaTable;
    LuaKey        key;
    int           stackTop;
};

template<typename T>
bool operator==(T value, LuaTableNode node)
{
  return value == (T) node;
}

template<typename T>
bool operator!=(T value, LuaTableNode node)
{
  return value != (T) node;
}

template<> int LuaTableNode::getDefault<int>(const int &default_value);
template<> long long LuaTableNode::getDefault<long long>(
    const long long &default_value);
template<> unsigned LuaTableNode::getDefault<unsigned>(
    const unsigned &default_value);
template<> unsigned long long LuaTableNode::getDefault<unsigned long long>(
    const unsigned long long &default_value);
template<> bool LuaTableNode::getDefault<bool>(const bool &default_value);
template<> double LuaTableNode::getDefault<double>(const double &default_value);
template<> float LuaTableNode::getDefault<float>(const float &default_value);
template<> std::string LuaTableNode::getDefault<std::string>(
    const std::string &default_value);

template<> void LuaTableNode::set<int>(const int &value);
template<> void LuaTableNode::set<long long>(const long long &value);
template<> void LuaTableNode::set<unsigned>(const unsigned &value);
template<> void LuaTableNode::set<unsigned long long>(
    const unsigned long long &value);
template<> void LuaTableNode::set<bool>(const bool &value);
template<> void LuaTableNode::set<float>(const float &value);
template<> void LuaTableNode::set<double>(const double &value);
template<> void LuaTableNode::set<std::string>(const std::string &value);

/* LUATABLES_H */
#endif
