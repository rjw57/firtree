// FIRTREE - A generic image processing library
// Copyright (C) 2007, 2008 Rich Wareham <richwareham@gmail.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License verstion as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 51
// Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

//=============================================================================
/// \file lru-cache.h A Least Recently Used cache.
//=============================================================================

//=============================================================================
#ifndef FIRTREE_LRU_CACHE_H
#define FIRTREE_LRU_CACHE_H
//=============================================================================

#include <stdlib.h>
#include <sys/time.h>

#include <map>

#include <firtree/main.h>

namespace Firtree { namespace Internal {

//=============================================================================
/// \brief A simple LRU cache. 
///    
/// This cache is unusual in that it has two parameters, ideal cache size and
/// maximum age. Each entry in the cache is associated with the wall-clock
/// time when it was last accessed. When an attempt is made to grow the
/// cache beyond the ideal cache size, any items with a time since last
/// access greater than the maximum age are removed in an oldest first manner
/// until the cache is the ideal size once again. Should there not be enough
/// elderly items to shrink the cache sufficiently, the cache remains 
/// 'overfull'.
///
/// The reason for this strategy is to catch the two common use patterns:
/// a small number of long-lived objects will remain in the cache. A 
/// large number of short-lived objects will be removed in an LRU fashion.   
template<typename Key>    
class LRUCache 
{
    private:    
        /// A cache entry. An object and its last access time.
        struct CacheEntry {
            ReferenceCounted*   Object;
            uint64_t            AccessTime;
        };

        /// The cache itself.
        typedef typename std::map<Key, CacheEntry> CacheMap;

        /// An iterator for the cache.
        typedef typename std::map<Key, CacheEntry>::iterator CacheMapIterator;

        /// A 'callable' object which can be used to sort ages from
        /// eldest to youngest.
        struct ageComp : public std::binary_function<
                         std::pair<Key,uint64_t>,
                         std::pair<Key,uint64_t>,
                         bool>
        {
            bool operator()(std::pair<Key,uint64_t> a,
                    std::pair<Key,uint64_t> b)
            {
                return a.second > b.second;
            }
        };

    public:
        /// Construct an LRU cache with an ideal size of 'size' and a 
        /// maximum age of 'age' milliseconds.
        LRUCache(uint32_t size, uint32_t age)
            :   m_IdealSize(size)
            ,   m_MaximumAge(age)
        {
            gettimeofday(&m_ConstructionTime, NULL);
        }    

        virtual ~LRUCache()
        {
            Purge();
        }

        // ====================================================================
        // CONST METHODS

        // ====================================================================
        /// Return the current size of the cache.
        uint32_t GetCacheSize() const 
        {
            return m_CacheMap.size();
        }

        // ====================================================================
        /// Return true if this cache has an entry corresponding to the
        /// key 'key'.
        bool Contains(Key key) const 
        {
            return (m_CacheMap.count(key) > 0);
        }

        // ====================================================================
        /// Return the entry corresponding to 'key' or NULL if there is no
        /// such entry. This might be released on the next call to Sweep()
        /// or SetEntryForKey() so it should be Retain()-ed if it is to
        /// ne used long-term.
        ReferenceCounted* GetEntryForKey(Key key)
        {
            if(!Contains(key)) 
                return NULL;

            m_CacheMap[key].AccessTime = Now();

            return m_CacheMap[key].Object;
        }

        // ====================================================================
        // MUTATING METHODS

        // ====================================================================
        /// Sweep through the cache, removing entries to reduce the size
        /// to m_IdealSize if possible.
        void Sweep()
        {
            // Don't do anything if the cache is small enough.
            if(GetCacheSize() <= m_IdealSize)
                return;

            // Get an idea of now for determining the age of entries.
            uint64_t now = Now();

            // Form a list of possible entries to remove.
            std::vector<std::pair<Key, uint64_t> > removableEntries;

            for(CacheMapIterator i = m_CacheMap.begin();
                    i != m_CacheMap.end(); i++)
            {
                const Key& key = (*i).first;
                CacheEntry& entry = (*i).second;

                uint64_t age = now - entry.AccessTime;

                if(age > m_MaximumAge)
                {
                    removableEntries.push_back(
                            std::pair<Key, uint64_t>(key, age));
                }
            }

            // If there are no removable entries, deal.
            if(removableEntries.size() == 0)
                return;

            // Sort the removable entries list in order of age 
            // from eldest to youngest.
            sort(removableEntries.begin(), removableEntries.end(),
                    ageComp());

            int numToRemove = GetCacheSize() - m_IdealSize;
            if(numToRemove > removableEntries.size())
            {
                numToRemove = removableEntries.size();
            }

            // Remove the entries
            for(int idx = 0; idx < numToRemove; idx++)
            {
                RemoveEntryForKey(removableEntries[idx].first);
            }
        }

        // ====================================================================
        /// Purge the cache of all entries.
        void Purge()
        {
            for(CacheMapIterator i = m_CacheMap.begin();
                    i != m_CacheMap.end(); i++)
            {
                CacheEntry& entry = (*i).second;

                // Release the object held in this entry.
                FIRTREE_SAFE_RELEASE(entry.Object);
            }

            m_CacheMap.clear();
        }

        // ====================================================================
        /// Remove any existing entry for the key 'key' should
        /// it exist. Silently do nothing otherwise.
        void RemoveEntryForKey(Key key)
        {
            if(!Contains(key))
                return;

            FIRTREE_SAFE_RELEASE(m_CacheMap[key].Object);
            m_CacheMap.erase(key);
        }

        // ====================================================================
        /// Add an entry to the cache with the key 'key'.
        void SetEntryForKey(ReferenceCounted* value, Key key)
        {
            RemoveEntryForKey(key);

            CacheEntry newEntry;
            newEntry.AccessTime = Now();
            newEntry.Object = value;
            FIRTREE_SAFE_RETAIN(newEntry.Object);

            m_CacheMap[key] = newEntry;

            Sweep();
        }

    private:    
        /// Return 'now' in milliseconds
        uint64_t Now() const 
        {
            struct timeval now;
            gettimeofday(&now, NULL);

            uint64_t delta = 0;
            delta = 1000ll *
                (uint64_t)(now.tv_sec - m_ConstructionTime.tv_sec);
            delta += 
                (uint64_t)(now.tv_usec - m_ConstructionTime.tv_usec) / 1000;

            return delta;
        }

        /// The time of class construction.
        struct timeval          m_ConstructionTime;

        /// The cache itself is a map from key to entry.
        CacheMap                m_CacheMap;

        /// The ideal size of the cache.
        uint32_t                m_IdealSize;

        /// The maximum age of cache entries.
        uint32_t                m_MaximumAge;
};

} }

//=============================================================================
#endif // FIRTREE_LRU_CACHE_H
//=============================================================================

//=============================================================================
// vim:sw=4:ts=4:cindent:et

