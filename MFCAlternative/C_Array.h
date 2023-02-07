#pragma once

template<typename T>
class C_Array
{
public:
    INT_PTR GetCount() const
    {
        return (INT_PTR)m_data.size();
    }

    INT_PTR Add(T newElement)
    {
        m_data.push_back(newElement);
        return this->GetCount() - 1;
    }

    T& GetAt(INT_PTR nIndex)
    {
        ASSERT(nIndex < this->GetCount());
        return m_data[nIndex];
    }

    const T& GetAt(INT_PTR nIndex) const
    {
        ASSERT(nIndex < this->GetCount());
        return m_data[nIndex];
    }

    void RemoveAt(INT_PTR nIndex, INT_PTR nCount = 1)
    {
        ASSERT(nIndex < this->GetCount());
        m_data.erase(m_data.begin() + nIndex, m_data.begin() + nIndex + nCount);
    }

    void RemoveAll()
    {
        m_data.clear();
    }

    void Sort()
    {
        std::sort(m_data.begin(), m_data.end());
    }

    T& operator[](INT_PTR nIndex)
    {
        return this->GetAt(nIndex);
    }

    const T& operator[](INT_PTR nIndex) const
    {
        return this->GetAt(nIndex);
    }

private:
    std::vector<T> m_data;
};