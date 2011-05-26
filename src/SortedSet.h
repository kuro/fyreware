
/**
 * @file SortedSet.h
 * @brief SortedSet definition
 */

#pragma once

#include <QMap>

template<typename T>
class SortedSet
{
private:
    QMap<T, QHashDummyValue> map;

public:
    inline SortedSet<T> () { }

    inline ~SortedSet () { }

    inline
    int size () const
    {
        return map.size();
    }

    inline
    bool isEmpty () const
    {
        return map.isEmpty();
    }

    inline
    int indexOf (const T& v) const
    {
        return map.keys().indexOf(v);
    }

    inline
    SortedSet& operator<< (const T& v)
    {
        map.insert(v, QHashDummyValue());
        return *this;
    }

    inline
    T& operator[] (int index)
    {
        return map.keys()[index];
    }

    inline
    const T& operator[] (int index) const
    {
        return map.keys()[index];
    }

    inline
    QList<T> toList () const
    {
        return map.keys();
    }

    inline
    const T& at (int idx) const
    {
        return map.keys().at(idx);
    }
};
