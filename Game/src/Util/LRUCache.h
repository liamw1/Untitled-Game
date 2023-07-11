#pragma once

template<typename K, typename V>
class LRUCache
{
public:
  using iterator = std::list<std::pair<K, V>>::iterator;

  LRUCache(int size)
    : m_Size(size) {}

  iterator begin() { return m_MostRecentlyUsed.begin(); }
  iterator end() { return m_MostRecentlyUsed.end(); }

  std::pair<iterator, bool> insert(K key, V&& value)
  {
    auto mapPosition = m_Map.find(key);
    if (mapPosition != m_Map.end())
    {
      iterator listPosition = mapPosition->second;
      setAsMostRecentlyUsed(listPosition);
      return { listPosition, false };
    }

    m_MostRecentlyUsed.push_front({ key, std::move(value) });
    m_Map.insert({ key, m_MostRecentlyUsed.begin() });
    if (m_MostRecentlyUsed.size() > m_Size)
    {
      m_Map.erase(m_MostRecentlyUsed.back().first);
      m_MostRecentlyUsed.pop_back();
    }
    return { m_MostRecentlyUsed.begin(), true };
  }

  iterator find(K key)
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