#ifndef SIMPLE_MAP_HPP
#define SIMPLE_MAP_HPP

template<typename Key, typename Value>
class Map
{
    struct Node
    {
        Key key;
        Value value;
        Node *left;
        Node *right;

        Node(const Key &k, const Value &v)
            : key(k), value(v), left(0), right(0)
        {}
    };

    Node *root;

    Node *insert_node(Node *node, const Key &k, const Value &v)
    {
        if (!node) return new Node(k, v);
        if (k < node->key)
            node->left = insert_node(node->left, k, v);
        else if (node->key < k)
            node->right = insert_node(node->right, k, v);
        else
            node->value = v; // overwrite
        return node;
    }

    Node *find_node(Node *node, const Key &k) const
    {
        if (!node) return 0;
        if (k < node->key)
            return find_node(node->left, k);
        else if (node->key < k)
            return find_node(node->right, k);
        else
            return node;
    }

    Node *&find_or_insert_node(Node *&node, const Key &k)
    {
        if (!node) {
            node = new Node(k, Value());  // insert default value
            return node;
        }
        if (k < node->key)
            return find_or_insert_node(node->left, k);
        else if (node->key < k)
            return find_or_insert_node(node->right, k);
        else
            return node;
    }

    void delete_tree(Node *node)
    {
        if (!node) return;
        delete_tree(node->left);
        delete_tree(node->right);
        delete node;
    }

public:
    Map() : root(0) {}
    ~Map() { delete_tree(root); }

    void insert(const Key &k, const Value &v)
    {
        root = insert_node(root, k, v);
    }

    Value *find(const Key &k) const
    {
        Node *n = find_node(root, k);
        return n ? &(n->value) : 0;
    }

    bool contains(const Key &k) const
    {
        return find_node(root, k) != 0;
    }

    bool isEmpty() const
    {
        return root == 0;
    }

    // --- operator[] (non-const) ---
    Value &operator[](const Key &k)
    {
        Node *n = find_or_insert_node(root, k);
        return n->value;
    }

    // (Optional) const version – only if you want it to throw or assert when not found.
    // const Value& operator[](const Key& k) const {
    //     Node* n = find_node(root, k);
    //     if (!n) { /* throw or assert here */ }
    //     return n->value;
    // }
};

#endif // SIMPLE_MAP_HPP
