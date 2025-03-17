#pragma once
#include "Handle.h"
#include "Device.h"
#include "Enums.h"
#include <vector>

namespace vg
{
    class QueryPool
    {
    public:
        QueryPool(QueryType type, uint32_t queryCount);

        QueryPool();
        QueryPool(QueryPool&& other) noexcept;
        QueryPool(const QueryPool& other) = delete;
        ~QueryPool();

        QueryPool& operator=(QueryPool&& other) noexcept;
        QueryPool& operator=(const QueryPool& other) = delete;
        operator const QueryPoolHandle& () const;

        void GetResults(int queryCount, int firstQuery, void* data, uint32_t dataSize, uint32_t stride, Flags<QueryResult> resultFlags = {});

        template<typename T>
        std::vector<T> GetResults(int queryCount, int firstQuery = 0, Flags<QueryResult> resultFlags = {}, uint32_t writeByteOffset = 0)
        {
            std::vector<T> results(queryCount);
            GetResults(queryCount, firstQuery, ((char*) results.data()) + writeByteOffset, results.size() * sizeof(T), sizeof(T), resultFlags);

            return results;
        }
        // template<typename... T>
        // std::tuple<T...> GetResults(int queryCount, int firstQuery = 0, Flags<QueryResult> resultFlags = {}, uint32_t writeByteOffset = 0)
        // {
        //     std::tuple<T...> results;
        //     GetResults(queryCount, firstQuery, ((char*) &results) + writeByteOffset, sizeof(results), sizeof(std::get<0>(results)), resultFlags);

        //     return results;
        // }
    private:
        QueryPoolHandle m_handle;
    };
}