#ifndef QT_VERSION

#ifndef QBYTEARRAY_H
#define QBYTEARRAY_H

#include <stddef.h>

class QByteArray {
    char* m_buf;
    size_t m_capacity;
    size_t m_pos;

   public:
    QByteArray(size_t size);
    ~QByteArray();

    inline char* data() { return m_buf; }
    inline void clear() { m_pos = 0; }

    inline const char* constData() const { return m_buf; }

    inline size_t size() { return m_pos; }

    bool seek(size_t n);

    inline bool isEmpty() { return m_pos == 0; }

    bool contains(char ch);
    bool contains(const char* str);
    bool endsWith(char ch);
    QByteArray toHex();
    int indexOfFourHexValues();

    bool append(char* data, int len = -1);
};

#endif  // QBYTEARRAY_H

#endif
