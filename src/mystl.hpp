#pragma once

#include <algorithm>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace my
{
    class string
    {
    private:
        // Fields
        std::string m_data;
    public:
        // Constructor, Copy/Move Constructors, Destructor
        string() : m_data("") {}
        string(const char* str) : m_data(str) {}
        string(const std::string& str) : m_data(str) {}
        string(const my::string& str) : m_data(str.m_data) {}
        string(my::string&& str) noexcept : m_data(std::move(str.m_data)) {}
        ~string() = default;

        // Operators
        inline operator std::string() const { return m_data; }
        inline string& operator=(const string& other)
        {
            m_data = other.m_data;
            return *this;
        }
        inline string& operator=(string&& other) noexcept 
        {
            m_data = std::move(other.m_data);
            return *this;
        }
        inline string& operator+=(const string& other)
        {
            m_data += other.m_data;
            return *this;
        }
        inline string& operator<<(const string& other)
        {
            m_data += other.m_data;
            return *this;
        }
        inline friend std::ostream& operator<<(std::ostream& os, const string& other)
        {
            os << other.m_data;
            return os;
        }
        inline char& operator[](int index)
        {
            if (index >= 0) {
                if (index >= m_data.size()) throw std::out_of_range("Index out of bounds");
                return m_data[index];
            }
            do {
                index = m_data.size() + index;
            } while (index < 0);
            return m_data[index];
        }
        inline const char& operator[](int index) const
        {
            size_t size = m_data.size();
            if (index >= 0) {
                if (index >= size) throw std::out_of_range("Index out of bounds");
                return m_data[index];
            }
            do {
                index = size + index;
            } while (index < 0);
            return m_data[index];
        }

        // Methods
        inline const std::string str() const { return m_data; }
        inline std::string str() { return m_data; }
        inline bool contains(const std::string& substr) const
        {
            return m_data.find(substr) != std::string::npos;
        }
        inline string ltrim() const
        {
            size_t start = m_data.find_first_not_of(" \t\n\r");
            return (start == std::string::npos) ? "" : m_data.substr(start);
        }
        inline string rtrim() const
        {
            size_t end = m_data.find_last_not_of(" \t\n\r");
            return (end == std::string::npos) ? "" : m_data.substr(0, end + 1);
        }
        inline string trim() const
        {
            size_t start = m_data.find_first_not_of(" \t\n\r");
            size_t end = m_data.find_last_not_of(" \t\r\n");
            return (start == std::string::npos) ? "" : m_data.substr(start, end - start + 1);
        }
        inline string to_upper() const
        {
            string result = m_data;
            std::transform(result.m_data.begin(), result.m_data.end(), result.m_data.begin(), ::toupper);
            return result;
        }
        inline string to_lower() const
        {
            string result = m_data;
            std::transform(result.m_data.begin(), result.m_data.end(), result.m_data.begin(), ::tolower);
            return result;
        }
        inline string to_upper(size_t from) const
        {
            string result = m_data;
            std::transform(result.m_data.begin()+from, result.m_data.end(), result.m_data.begin()+from, ::toupper);
            return result;
        }
        inline string to_lower(size_t from) const
        {
            string result = m_data;
            std::transform(result.m_data.begin()+from, result.m_data.end(), result.m_data.begin()+from, ::tolower);
            return result;
        }
        inline string to_upper(size_t from, size_t to) const
        {
            string result = m_data;
            std::transform(result.m_data.begin()+from, result.m_data.begin()+to, result.m_data.begin()+from, ::toupper);
            return result;
        }
        inline string to_lower(size_t from, size_t to) const
        {
            string result = m_data;
            std::transform(result.m_data.begin()+from, result.m_data.begin()+to, result.m_data.begin()+from, ::tolower);
            return result;
        }
        inline std::vector<string> split(string delim) const 
        {
            std::string copy = this->m_data;
            std::vector<string> result;
            my::string temp; 
            while (copy.find(delim.str()) != std::string::npos) {
                temp = copy.substr(0, copy.find(delim.str()));
                result.emplace_back(temp);
                copy.erase(copy.begin(), copy.begin()+(copy.find(delim.str())+1));
            }
            if (copy.size() > 0) result.emplace_back(copy);
            return result;
        }

        // Forward std::string methods
        inline size_t size() const { return m_data.size(); }
        inline bool empty() const { return m_data.empty(); }
        inline void push(const char c) { m_data.push_back(c); }
    };
};

namespace my
{
    template <typename T>
    class vector
    {
    private:
        // Fields
        std::vector<T> m_data;
    public:
        // Constructor, Copy/Move Constructors, Destructor
        vector() : m_data() {}
        vector(const vector<T>& vec) : m_data(vec.begin(), vec.end()) {}
        vector(vector<T>&& vec) noexcept : m_data(std::move(vec.m_data)) {}
        vector(const std::vector<T>& elements) : m_data(elements.begin(), elements.end()) {}
        vector(const std::initializer_list<T>& elements) : m_data(elements.begin(), elements.end()) {}

        // Operators
        inline operator std::vector<T>() const { return m_data; }
        inline vector& operator=(const vector<T>& vec)
        {
            if (this != &vec) {
                vector temp(vec);
                std::swap(this->m_data, temp.m_data);
            }
            return *this;
        }
        inline vector& operator=(vector<T>&& vec) noexcept
        {
            if (this != &vec) {
                m_data = std::move(vec.m_data);
            }
            return *this;
        }
        inline T& operator[](int index)
        {
            if (index >= 0) {
                if (index >= this->size()) throw std::out_of_range("Index out of bounds");
                return m_data[index];
            }
            do {
                index = this->size() + index;
            } while (index < 0);
            return m_data[index];
        }
        inline const T& operator[](int index) const
        {
            if (index >= 0) {
                if (index >= this->size()) throw std::out_of_range("Index out of bounds");
                return m_data[index];
            }
            do {
                index = this->size() + index;
            } while (index < 0);
            return m_data[index];
        }
        inline friend std::ostream& operator<<(std::ostream& os, my::vector<T>& items)
        {
            for (T& item : items) {
                os << item << std::endl;
            }
            return os;
        }

        // Methods
        template <typename... Args>
        inline void emplace(Args&&... args) { m_data.emplace_back(std::forward<Args>(args)...); }
        inline void push(const T& value) { m_data.push_back(value); }
        inline void push(T&& value) { m_data.push_back(value); }
        inline T pop()
        {
            if (this->size() == 0) throw std::length_error("Vector is empty.");
            T result = m_data[m_data.size()-1];
            m_data.pop_back();
            return result;
        }
        template <typename U = T>
        inline std::enable_if_t
        <
            std::is_same_v<U, my::string> || std::is_same_v<U, std::string>, 
            my::string
        >
        join(const my::string& delim) const {
            my::string result;
            for (size_t i = 0; i < m_data.size(); ++i) {
                result += m_data[i];
                if (i < m_data.size() - 1) {
                    result += delim;
                }
            }
            return result;
        }
        inline size_t size() const { return m_data.size(); }
        inline size_t capacity() const { return m_data.capacity(); }
        inline auto begin() -> decltype(m_data.begin()) { return m_data.begin(); }
        inline auto end() -> decltype(m_data.end()) { return m_data.end(); }
        inline auto begin() const -> decltype(m_data.begin()) { return m_data.begin(); }
        inline auto end() const -> decltype(m_data.end()) { return m_data.end(); }
        inline auto cbegin() const -> decltype(m_data.cbegin()) { return m_data.cbegin(); }
        inline auto cend() const -> decltype(m_data.cend()) { return m_data.cend(); }
        inline auto rbegin() -> decltype(m_data.rbegin()) { return m_data.rbegin(); }
        inline auto rend() -> decltype(m_data.rend()) { return m_data.rend(); }
        inline auto rbegin() const -> decltype(m_data.rbegin()) { return m_data.rbegin(); }
        inline auto rend() const -> decltype(m_data.rend()) { return m_data.rend(); }
    };
}

