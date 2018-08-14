#ifndef QT_VERSION

#include "qbytearray.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

QByteArray::QByteArray(size_t size) {
    m_buf = (char*)malloc(size + 1);  // pad for NULL termination
    m_buf[0] = '\0';

    m_capacity = size;
    m_pos = 0;
}

QByteArray::~QByteArray() { free(m_buf); }

bool QByteArray::seek(size_t n) {
    if (n > m_capacity) {
        size_t new_capacity = n;
        char* new_buf = (char*)realloc(m_buf, new_capacity + 1);
        if (new_buf == NULL) return false;

        m_buf = new_buf;
        m_capacity = new_capacity;
    }

    m_pos = n;
    m_buf[m_pos] = '\0';
    return true;
}

bool QByteArray::contains(char ch) {
    void* p = memchr(m_buf, ch, m_pos);
    return p != NULL;
}

bool QByteArray::contains(const char* str) {
    void* p = memmem(m_buf, m_pos, str, strlen(str));
    return p != NULL;
}

bool QByteArray::endsWith(char ch) {
    if (m_pos < 1) return false;
    return m_buf[m_pos - 1] == ch;
}

QByteArray QByteArray::toHex() {
    QByteArray ret(m_pos * 2);
    const char* val = "0123456789ABCDEF";

    size_t j = 0;
    for (size_t i = 0; i != m_pos; ++i) {
        ret.m_buf[j++] = val[(m_buf[i] >> 4) & 0xF];
        ret.m_buf[j++] = val[(m_buf[i] & 0xF)];
    }
    ret.m_buf[j] = '\0';
    return ret;
}

int QByteArray::indexOfFourHexValues() {
    const char* val = "0123456789ABCDEFabcdef";
    size_t len = strlen(val);

    int ret = -1;
    int k = 0;

    for (size_t i = 0; i != m_pos; ++i) {
        if (memchr(val, m_buf[i], len) != NULL) {
            if (k == 0) ret = i;
            if (++k == 8) break;
        } else {
            k = 0;
        }
    }

    if (k == 8) return ret;
    return -1;
}

bool QByteArray::append(char* data, int len) {
    if (len == -1) {
        len = strlen(data);
    }

    size_t new_capacity = m_pos + len;
    if (new_capacity > m_capacity) {
        char* new_buf = (char*)realloc(m_buf, new_capacity + 1);
        if (new_buf == NULL) return false;

        m_buf = new_buf;
        m_capacity = new_capacity;
    }

    memcpy(m_buf + m_pos, data, len);
    m_pos += len;
    m_buf[m_pos] = '\0';
    return true;
}

#endif
