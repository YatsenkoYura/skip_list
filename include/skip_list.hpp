#pragma once

#include <algorithm>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <random>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include <concepts> // добавлено

/**
 * @brief A skip list implementation for sorted storage and fast lookup.
 * @tparam T        Type of elements stored.
 * @tparam Compare  Comparison functor (defaults to std::less<T>).
 */
template <typename T, typename Compare = std::less<T>>
class skip_list {
private:
    struct Node {
        T value;
        std::vector<Node*> forward;

        Node(int level, const T& val)
                : value(val), forward(level + 1, nullptr) {}
    };

    static constexpr int MAX_LEVEL = 16;
    static constexpr double P = 0.5;

    Node* head_;
    int level_;
    size_t size_;
    Compare cmp_;
    std::mt19937 gen_;
    std::uniform_real_distribution<> dist_;

    /**
     * @brief Generate a random level for a new node.
     * @return Level in range [0, MAX_LEVEL].
     */
    int random_level() {
        int lvl = 0;
        while (lvl < MAX_LEVEL && dist_(gen_) < P)
            ++lvl;
        return lvl;
    }

public:
    using value_type        = T;
    using size_type         = size_t;
    using difference_type   = std::ptrdiff_t;
    using reference         = T&;
    using const_reference   = const T&;
    using key_type          = T;
    using key_compare       = Compare;

    /**
     * @brief Iterator for skip_list.
     * @tparam IsConst  true for const_iterator, false for iterator.
     */
    template <bool IsConst>
    class basic_iterator {
        friend class skip_list;
        using node_ptr = std::conditional_t<IsConst, const Node*, Node*>;
        node_ptr ptr_;

        explicit basic_iterator(node_ptr p) : ptr_(p) {}

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using reference         = std::conditional_t<IsConst, const T&, T&>;
        using pointer           = std::conditional_t<IsConst, const T*, T*>;

        basic_iterator() : ptr_(nullptr) {}

        // ВАЖНО: Здесь заменено enable_if на requires!
        template <bool B = IsConst>
        requires B
        basic_iterator(basic_iterator<false> const& other) noexcept
                : ptr_(other.ptr_) {}

        basic_iterator& operator++() {
            if (ptr_) ptr_ = ptr_->forward[0];
            return *this;
        }

        basic_iterator operator++(int) {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        reference operator*() const {
            if (!ptr_) throw std::out_of_range("SkipList iterator out of range");
            return ptr_->value;
        }

        pointer operator->() const {
            return std::addressof(operator*());
        }

        bool operator==(basic_iterator const& o) const noexcept {
            return ptr_ == o.ptr_;
        }

        bool operator!=(basic_iterator const& o) const noexcept {
            return ptr_ != o.ptr_;
        }
    };

    using iterator       = basic_iterator<false>;
    using const_iterator = basic_iterator<true>;

    /** @brief Default constructor. */
    skip_list()
            : head_(new Node(MAX_LEVEL, T{})),
              level_(0),
              size_(0),
              cmp_(Compare{}),
              gen_(std::random_device{}()),
              dist_(0.0, 1.0) {}

    /**
     * @brief Construct from initializer list.
     * @param il List of values to insert.
     */
    skip_list(std::initializer_list<T> il) : skip_list() {
        for (auto const& v : il) insert(v);
    }

    /**
     * @brief Construct from input iterators.
     * @tparam InputIt  Input iterator type.
     * @param first     Begin iterator.
     * @param last      End iterator.
     */
    template <typename InputIt>
    requires std::input_iterator<InputIt>
    skip_list(InputIt first, InputIt last) : skip_list() {
        for (; first != last; ++first)
            insert(*first);
    }

    /** @brief Copy constructor. */
    skip_list(const skip_list& other) : skip_list() {
        for (auto const& v : other)
            insert(v);
    }

    /** @brief Copy assignment. */
    skip_list& operator=(const skip_list& other) {
        if (this != &other) {
            clear();
            for (auto const& v : other)
                insert(v);
        }
        return *this;
    }

    /** @brief Move constructor. */
    skip_list(skip_list&& o) noexcept
            : head_(o.head_),
              level_(o.level_),
              size_(o.size_),
              cmp_(std::move(o.cmp_)),
              gen_(std::random_device{}()),
              dist_(0.0, 1.0) {
        o.head_ = nullptr;
        o.size_ = 0;
    }

    /** @brief Move assignment. */
    skip_list& operator=(skip_list&& o) noexcept {
        if (this != &o) {
            clear();
            delete head_;
            head_    = o.head_;
            level_   = o.level_;
            size_    = o.size_;
            cmp_     = std::move(o.cmp_);
            o.head_  = nullptr;
            o.size_  = 0;
        }
        return *this;
    }

    /** @brief Destructor. */
    ~skip_list() {
        if (head_) {
            clear();
            delete head_;
        }
    }

    /** @brief Check if empty. */
    bool empty() const noexcept { return size_ == 0; }

    /** @brief Get number of elements. */
    size_type size() const noexcept { return size_; }

    /** @brief Remove all elements. */
    void clear() noexcept {
        if (!head_) return;

        Node* cur = head_->forward[0];
        while (cur) {
            Node* nxt = cur->forward[0];
            delete cur;
            cur = nxt;
        }
        std::fill(head_->forward.begin(), head_->forward.end(), nullptr);
        level_ = 0;
        size_  = 0;
    }

    /**
     * @brief Insert a value.
     * @param value  Value to insert.
     * @return Pair(iterator to new or existing element, true if inserted).
     */
    std::pair<iterator, bool> insert(const T& value) {
        std::vector<Node*> update(MAX_LEVEL + 1);
        Node* x = head_;
        for (int i = level_; i >= 0; --i) {
            while (x->forward[i] && cmp_(x->forward[i]->value, value))
                x = x->forward[i];
            update[i] = x;
        }
        x = x->forward[0];
        if (x && !cmp_(value, x->value) && !cmp_(x->value, value)) {
            return {iterator(x), false};
        }
        int lvl = random_level();
        if (lvl > level_) {
            for (int i = level_ + 1; i <= lvl; ++i)
                update[i] = head_;
            level_ = lvl;
        }
        Node* newNode = new Node(lvl, value);
        for (int i = 0; i <= lvl; ++i) {
            newNode->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = newNode;
        }
        ++size_;
        return {iterator(newNode), true};
    }

    /**
     * @brief Emplace a value (perfect-forwarded).
     * @param args  Arguments to construct T.
     * @return Pair(iterator, true if inserted).
     */
    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        T tmp(std::forward<Args>(args)...);
        return insert(tmp);
    }

    /**
     * @brief Remove value.
     * @param value  Value to erase.
     * @return Number of elements removed (0 or 1).
     */
    size_type erase(const T& value) {
        std::vector<Node*> update(MAX_LEVEL + 1);
        Node* x = head_;
        for (int i = level_; i >= 0; --i) {
            while (x->forward[i] && cmp_(x->forward[i]->value, value))
                x = x->forward[i];
            update[i] = x;
        }
        x = x->forward[0];
        if (!x || cmp_(value, x->value) || cmp_(x->value, value))
            return 0;
        for (int i = 0; i <= level_; ++i) {
            if (update[i]->forward[i] != x) break;
            update[i]->forward[i] = x->forward[i];
        }
        delete x;
        while (level_ > 0 && head_->forward[level_] == nullptr)
            --level_;
        --size_;
        return 1;
    }

    /**
     * @brief Find element.
     * @param value  Key to find.
     * @return Iterator to element or end().
     */
    iterator find(const T& value) {
        Node* x = head_;
        for (int i = level_; i >= 0; --i) {
            while (x->forward[i] && cmp_(x->forward[i]->value, value))
                x = x->forward[i];
        }
        x = x->forward[0];
        if (x && !cmp_(value, x->value) && !cmp_(x->value, value))
            return iterator(x);
        return end();
    }

    /** @copydoc find(const T&) */
    const_iterator find(const T& value) const {
        return const_cast<skip_list*>(this)->find(value);
    }

    /**
     * @brief Get first element not less than key.
     * @param key  Search key.
     * @return Lower bound iterator.
     */
    iterator lower_bound(const T& key) {
        Node* x = head_;
        for (int i = level_; i >= 0; --i) {
            while (x->forward[i] && cmp_(x->forward[i]->value, key))
                x = x->forward[i];
        }
        return iterator(x->forward[0]);
    }

    /** @copydoc lower_bound(const T&) */
    const_iterator lower_bound(const T& key) const {
        return const_cast<skip_list*>(this)->lower_bound(key);
    }

    /**
     * @brief Get first element greater than key.
     * @param key  Search key.
     * @return Upper bound iterator.
     */
    iterator upper_bound(const T& key) {
        Node* x = head_;
        for (int i = level_; i >= 0; --i) {
            while (x->forward[i] && !cmp_(key, x->forward[i]->value))
                x = x->forward[i];
        }
        return iterator(x->forward[0]);
    }

    /** @copydoc upper_bound(const T&) */
    const_iterator upper_bound(const T& key) const {
        return const_cast<skip_list*>(this)->upper_bound(key);
    }

    /**
     * @brief Count occurrences of key.
     * @param key  Search key.
     * @return 1 if found, 0 otherwise.
     */
    size_type count(const T& key) const {
        return find(key) != end() ? 1 : 0;
    }

    /**
     * @brief Check if key exists.
     * @param key  Search key.
     * @return true if found.
     */
    bool contains(const T& key) const {
        return count(key) != 0;
    }

    /** @brief Iterator to first element. */
    iterator begin() noexcept {
        return iterator(head_->forward[0]);
    }

    /** @copydoc begin() */
    const_iterator begin() const noexcept {
        return const_iterator(head_->forward[0]);
    }

    /** @brief Const begin. */
    const_iterator cbegin() const noexcept {
        return const_iterator(head_->forward[0]);
    }

    /** @brief Iterator to end. */
    iterator end() noexcept {
        return iterator(nullptr);
    }

    /** @copydoc end() */
    const_iterator end() const noexcept {
        return const_iterator(nullptr);
    }

    /** @brief Const end. */
    const_iterator cend() const noexcept {
        return const_iterator(nullptr);
    }

    /**
     * @brief Equality comparison.
     * @param rhs  Other skip_list.
     * @return true if sizes and elements match.
     */
    bool operator==(skip_list const& rhs) const {
        if (size_ != rhs.size_) return false;
        auto it1 = begin();
        auto it2 = rhs.begin();
        while (it1 != end()) {
            if (*it1 != *it2) return false;
            ++it1; ++it2;
        }
        return true;
    }

    /** @brief Inequality comparison. */
    bool operator!=(skip_list const& rhs) const {
        return !(*this == rhs);
    }
};