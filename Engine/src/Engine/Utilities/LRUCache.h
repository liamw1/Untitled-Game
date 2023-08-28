#pragma once

namespace Engine
{
  template<Hashable K, typename V>
  class LRUCache
  {
  public:
    using iterator = std::list<std::pair<K, V>>::iterator;
  
    LRUCache(int size)
      : m_Size(size) {}
  
    iterator begin() { return m_MostRecentlyUsed.begin(); }
    iterator end() { return m_MostRecentlyUsed.end(); }

    V& operator[](const K& key)
    {
      auto [listPosition, insertionSuccess] = insert(key, V());
      if (!insertionSuccess)
        listPosition = find(key);

      EN_CORE_ASSERT(listPosition != end(), "Could not find or insert value at the given key!");
      return listPosition->second;
    }

    template<DecaysTo<V> T>
    std::pair<iterator, bool> insert(const K& key, T&& value)
    {
      auto mapPosition = m_Map.find(key);
      if (mapPosition != m_Map.end())
      {
        iterator listPosition = mapPosition->second;
        setAsMostRecentlyUsed(listPosition);
        return { listPosition, false };
      }
  
      m_MostRecentlyUsed.emplace_front(key, std::forward<T>(value));
      m_Map.emplace(key, m_MostRecentlyUsed.begin());
      if (m_MostRecentlyUsed.size() > m_Size)
      {
        m_Map.erase(m_MostRecentlyUsed.back().first);
        m_MostRecentlyUsed.pop_back();
      }
      return { m_MostRecentlyUsed.begin(), true };
    }
  
    iterator find(const K& key)
    {
      auto mapPosition = m_Map.find(key);
      if (mapPosition == m_Map.end())
        return m_MostRecentlyUsed.end();
  
      iterator listPosition = mapPosition->second;
      setAsMostRecentlyUsed(listPosition);
      return listPosition;
    }
  
  private:
    std::list<std::pair<K, V>> m_MostRecentlyUsed;
    std::unordered_map<K, iterator> m_Map;
    int m_Size;
  
    void setAsMostRecentlyUsed(iterator listPosition)
    {
      m_MostRecentlyUsed.splice(m_MostRecentlyUsed.begin(), m_MostRecentlyUsed, listPosition);
    }
  };
}