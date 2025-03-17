#pragma once
#include <VG/VG.h>
#include <memory>
#include <vector>
#include <iostream>

/// @brief Klasa działająca podobnie do std::vector ale do przechowywania danych w pamięci dostępnej przez GPU.
/// @tparam T typ danych
template <typename T>
class BufferArray
{
public:
    BufferArray() : buffer(nullptr), m_size(0), m_begin(nullptr) {}

    /// @brief Konstruktor rezerwujący region pamięci z zdefniniowanymi sposobami użycia.
    /// @param count ilość elementów, dla których jest rezerwowana pamieć. 
    /// @param usage zbiór możliwych sposobów użycia regionu pamięci.
    BufferArray(int count, vg::Flags<vg::BufferUsage> usage)
        : usage(usage), m_size(count)
    {
        this->usage.Set(vg::BufferUsage::TransferDst);
        this->usage.Set(vg::BufferUsage::TransferSrc);
        if (count == 0)
            count = 1;
        buffer.reset(new vg::Buffer(count * sizeof(T), usage));
        vg::Allocate(buffer.get(), { vg::MemoryProperty::HostVisible, vg::MemoryProperty::HostCoherent });
        m_begin = (T*) buffer.get()->MapMemory();
    }

    BufferArray(BufferArray&& rhs) noexcept
        :BufferArray()
    {
        *this = std::move(rhs);
    }

    BufferArray(const BufferArray& rhs)
        :BufferArray()
    {
        *this = rhs;
    }

    BufferArray& operator=(BufferArray&& rhs)
    {
        if (&rhs == this)return *this;

        std::swap(deviceBuffer, rhs.deviceBuffer);
        std::swap(buffer, rhs.buffer);
        std::swap(m_size, rhs.m_size);
        std::swap(m_begin, rhs.m_begin);
        std::swap(usage, rhs.usage);

        return *this;
    }

    BufferArray& operator=(const BufferArray& rhs)
    {
        if (&rhs == this)return *this;

        resize(rhs.size());
        memcpy(begin(), rhs.begin(), size() * sizeof(T));
        usage = rhs.usage;

        return *this;
    }

    /// @brief Rezerwuje odpowiedni blok pamięci.
    /// @param count wielkość rezerwacji
    void reserve(uint32_t count)
    {
        if (capacity() == count)
            return;

        vg::Buffer* newBuffer = new vg::Buffer(count * sizeof(T), usage);
        vg::Allocate(newBuffer, { vg::MemoryProperty::HostVisible, vg::MemoryProperty::HostCoherent });

        uint32_t minSize = newBuffer->GetSize();
        if (buffer->GetSize() < minSize)
            minSize = buffer->GetSize();
        memcpy(newBuffer->MapMemory(), this->data(), minSize);
        buffer.reset(newBuffer);
        m_begin = (T*) buffer.get()->MapMemory();
    }

    /// @brief Zmienia rozmiar buffera.
    /// @param count nowy rozmiar
    void resize(int count)
    {
        if (m_size == count)
            return;

        m_size = count;
        if (capacity() < m_size)
            reserve(m_size);
    }

    /// @brief Zwalnia nie wykorzystywaną pamięć.
    void shrink_to_fit()
    {
        reserve(m_size);
    }

    /// @brief Dodaje nowy element.
    /// @param element nowy element
    void push_back(const T& element)
    {
        m_size++;
        auto cap = capacity();
        if (cap < m_size)
            reserve((cap + 1) * 1.71f);
        data()[m_size - 1] = element;
    }

    /// @brief Wstawia nowy element.
    /// @param element nowy element
    void emplace_back(T&& element)
    {
        m_size++;
        auto cap = capacity();
        if (cap < m_size)
            reserve((cap + 1) * 1.71f);
        data()[m_size - 1] = std::move(element);
    }

    /// @brief Usuwa ostatni element.
    void pop_back()
    {
        m_size--;
    }

    /// @brief Usuwa element.
    /// @param index indeks elementu do usunięcia
    void erase(uint32_t index)
    {
        for (int i = index; i < m_size - 1; i++)
            data()[i] = data()[i + 1];
        pop_back();
    }

    /// @brief Wstawia nowy element przed pozycję @p index
    /// @param element nowy element
    /// @param index pozycja
    void insert(const T& element, uint32_t index)
    {
        m_size++;
        if (capacity() < m_size)
            reserve((capacity() + 1) * 1.71f);

        for (int i = m_size; i > index; i--)
            data()[i] = data()[i - 1];

        data()[index] = element;
    }

    /// @brief Wstawoa listę elementów przed pozycję @p index
    /// @param elements nowe elementy
    /// @param index pozycja
    void insert_range(std::span<const T> elements, uint32_t index)
    {
        m_size += elements.size();
        if (capacity() < m_size)
            reserve((capacity() + elements.size()) * 1.71f);

        for (int i = m_size; i > index; i--)
            data()[i] = data()[i - elements.size()];

        for (int i = index; i < index + elements.size(); i++)
            data()[i] = elements[i - index];
    }

    T& operator [](uint32_t index) { return data()[index]; }
    T& operator [](uint32_t index) const { return data()[index]; }

    uint32_t capacity() const { if (!buffer) return 0; return buffer.get()->GetSize() / sizeof(T); }
    uint32_t size() const { return m_size; }
    uint32_t byte_size() const { return m_size * sizeof(T); }
    T* begin() { return m_begin; }
    T* begin() const { return m_begin; }
    T* end() { return m_begin + m_size; }
    T* end() const { return m_begin + m_size; }
    T* data() { return m_begin; }
    T* data() const { return m_begin; }

    operator vg::Buffer& () const { if (deviceBuffer)return *deviceBuffer; return *buffer; }
    operator vg::BufferHandle() const { if (deviceBuffer)return *deviceBuffer; return *buffer; }

private:
    std::unique_ptr<vg::Buffer> deviceBuffer;
    std::unique_ptr<vg::Buffer> buffer;
    uint32_t m_size;
    T* m_begin;
    vg::Flags<vg::BufferUsage> usage;
};