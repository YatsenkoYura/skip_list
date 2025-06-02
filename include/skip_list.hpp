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

template <typename T, typename Compare = std::less<T>>
class skip_list {
private:
    struct Node {
        T value;
        std::vector<Node*> forward;
        Node(int level, const T& val) : value(val), forward(level + 1, nullptr) {}
    };

    static constexpr int MAX_LEVEL = 16;
    static constexpr double P = 0.5;

    Node* head_;
    int level_;
    size_t size_;
    Compare cmp_;
    std::mt19937 gen_;
    std::uniform_real_distribution<> dist_;

    int random_level() {
        int lvl = 0;
        while (lvl < MAX_LEVEL && dist_(gen_) < P) ++lvl;
        return lvl;
    }

public:
    using value_type = T;
    using size_type = size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using key_type = T;
    using key_compare = Compare;

    template <bool IsConst>
    class basic_iterator {
        friend class skip_list;
        using node_ptr = std::conditional_t<IsConst, const Node*, Node*>;
        node_ptr ptr_;

        explicit basic_iterator(node_ptr p) : ptr_(p) {}

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using reference = std::conditional_t<IsConst, const T&, T&>;
        using pointer = std::conditional_t<IsConst, const T*, T*>;

        basic_iterator() : ptr_(nullptr) {}

        template <bool B = IsConst, typename = std::enable_if_t<B>>
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
        pointer operator->() const { return std::addressof(operator*()); }

        bool operator==(basic_iterator const& o) const noexcept {
            return ptr_ == o.ptr_;
        }
        bool operator!=(basic_iterator const& o) const noexcept {
            return ptr_ != o.ptr_;
        }
    };

    using iterator = basic_iterator<false>;
    using const_iterator = basic_iterator<true>;

    skip_list()
            : head_(new Node(MAX_LEVEL, T{})),
              level_(0),
              size_(0),
              cmp_(Compare{}),
              gen_(std::random_device{}()),
              dist_(0.0, 1.0) {}

    skip_list(std::initializer_list<T> il) : skip_list() {
        for (auto const& v : il) insert(v);
    }

    template <typename InputIt,
            typename = std::enable_if_t<std::is_convertible_v<
                    typename std::iterator_traits<InputIt>::iterator_category,
                    std::input_iterator_tag>>>
    skip_list(InputIt first, InputIt last) : skip_list() {
        for (; first != last; ++first) insert(*first);
    }

    skip_list(const skip_list& other) : skip_list() {
        for (auto const& v : other) insert(v);
    }

    skip_list& operator=(const skip_list& other) {
        if (this != &other) {
            clear();
            for (auto const& v : other) insert(v);
        }
        return *this;
    }

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

    skip_list& operator=(skip_list&& o) noexcept {
        if (this != &o) {
            clear();
            delete head_;
            head_ = o.head_;
            level_ = o.level_;
            size_ = o.size_;
            cmp_ = std::move(o.cmp_);
            o.head_ = nullptr;
            o.size_ = 0;
        }
        return *this;
    }

    ~skip_list() {
        clear();
        delete head_;
    }

    bool empty() const noexcept { return size_ == 0; }
    size_type size() const noexcept { return size_; }
    void clear() noexcept {
        Node* cur = head_->forward[0];
        while (cur) {
            Node* nxt = cur->forward[0];
            delete cur;
            cur = nxt;
        }
        std::fill(head_->forward.begin(), head_->forward.end(), nullptr);
        level_ = 0;
        size_ = 0;
    }

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
            for (int i = level_ + 1; i <= lvl; ++i) update[i] = head_;
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

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        T tmp(std::forward<Args>(args)...);
        return insert(tmp);
    }

    size_type erase(const T& value) {
        std::vector<Node*> update(MAX_LEVEL + 1);
        Node* x = head_;
        for (int i = level_; i >= 0; --i) {
            while (x->forward[i] && cmp_(x->forward[i]->value, value))
                x = x->forward[i];
            update[i] = x;
        }
        x = x->forward[0];
        if (!x || cmp_(value, x->value) || cmp_(x->value, value)) return 0;
        for (int i = 0; i <= level_; ++i) {
            if (update[i]->forward[i] != x) break;
            update[i]->forward[i] = x->forward[i];
        }
        delete x;
        while (level_ > 0 && head_->forward[level_] == nullptr) --level_;
        --size_;
        return 1;
    }

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

    const_iterator find(const T& value) const {
        return const_cast<skip_list*>(this)->find(value);
    }

    iterator lower_bound(const T& key) {
        Node* x = head_;
        for (int i = level_; i >= 0; --i) {
            while (x->forward[i] && cmp_(x->forward[i]->value, key))
                x = x->forward[i];
        }
        return iterator(x->forward[0]);
    }

    const_iterator lower_bound(const T& key) const {
        return const_cast<skip_list*>(this)->lower_bound(key);
    }

    iterator upper_bound(const T& key) {
        Node* x = head_;
        for (int i = level_; i >= 0; --i) {
            while (x->forward[i] && !cmp_(key, x->forward[i]->value))
                x = x->forward[i];
        }
        return iterator(x->forward[0]);
    }

    const_iterator upper_bound(const T& key) const {
        return const_cast<skip_list*>(this)->upper_bound(key);
    }

    size_type count(const T& key) const { return find(key) != end() ? 1 : 0; }

    bool contains(const T& key) const { return count(key) != 0; }

    iterator begin() noexcept { return iterator(head_->forward[0]); }
    const_iterator begin() const noexcept {
        return const_iterator(head_->forward[0]);
    }
    const_iterator cbegin() const noexcept {
        return const_iterator(head_->forward[0]);
    }

    iterator end() noexcept { return iterator(nullptr); }
    const_iterator end() const noexcept { return const_iterator(nullptr); }
    const_iterator cend() const noexcept { return const_iterator(nullptr); }

    bool operator==(skip_list const& rhs) const {
        if (size_ != rhs.size_) return false;
        auto it1 = begin();
        auto it2 = rhs.begin();
        for (; it1 != end(); ++it1, ++it2)
            if (*it1 != *it2) return false;
        return true;
    }
    bool operator!=(skip_list const& rhs) const { return !(*this == rhs); }
};