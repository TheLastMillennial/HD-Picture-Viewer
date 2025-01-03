#pragma once

#include <types/pair.h>
#include <debug.h>

template <typename Key, typename T>
class Map
{
private:
	// Structure for the nodes of the tree
	struct Node
	{
		Pair<Key, T> data;
		Node *left;
		Node *right;
		Node *parent;
		bool color;  // 0 for red, 1 for black

		Node(const Pair<Key, T> &pair)
			: data(pair), left(nullptr), right(nullptr), parent(nullptr), color(0)
		{}
	};

	Node *root;

	template <typename T1, typename T2>
	static Pair<T1, T2> make_pair(const T1 &a, const T2 &b)
	{
		return Pair<T1, T2>(a, b);
	}

	// Default comparison function (ascending order)
	bool defaultCompare(const Key &a, const Key &b) const
	{
		return a < b;
	}

	// Function to perform a left rotation
	void leftRotate(Node *x)
	{
		Node *y = x->right;
		x->right = y->left;
		if (y->left != nullptr)
			y->left->parent = x;
		y->parent = x->parent;
		if (x->parent == nullptr)
			root = y;
		else if (x == x->parent->left)
			x->parent->left = y;
		else
			x->parent->right = y;
		y->left = x;
		x->parent = y;
	}

	// Function to perform a right rotation
	void rightRotate(Node *y)
	{
		Node *x = y->left;
		y->left = x->right;
		if (x->right != nullptr)
			x->right->parent = y;
		x->parent = y->parent;
		if (y->parent == nullptr)
			root = x;
		else if (y == y->parent->left)
			y->parent->left = x;
		else
			y->parent->right = x;
		x->right = y;
		y->parent = x;
	}

	// Function to fix violations after insertion
	void fixInsert(Node *k)
	{
		Node *u;
		while (k->parent && k->parent->color == 0) {
			if (k->parent == k->parent->parent->left) {
				u = k->parent->parent->right;
				if (u && u->color == 0) {
					k->parent->color = 1;
					u->color = 1;
					k->parent->parent->color = 0;
					k = k->parent->parent;
				}
				else {
					if (k == k->parent->right) {
						k = k->parent;
						leftRotate(k);
					}
					k->parent->color = 1;
					k->parent->parent->color = 0;
					rightRotate(k->parent->parent);
				}
			}
			else {
				u = k->parent->parent->left;
				if (u && u->color == 0) {
					k->parent->color = 1;
					u->color = 1;
					k->parent->parent->color = 0;
					k = k->parent->parent;
				}
				else {
					if (k == k->parent->left) {
						k = k->parent;
						rightRotate(k);
					}
					k->parent->color = 1;
					k->parent->parent->color = 0;
					leftRotate(k->parent->parent);
				}
			}
			if (k == root)
				break;
		}
		root->color = 1;
	}

	// Insert a node into the tree
	void insert(const Pair<Key, T> &pair)
	{
		Node *z = new Node(pair);
		Node *y = nullptr;
		Node *x = root;

		// Standard BST insert
		while (x != nullptr) {
			y = x;
			if (defaultCompare(z->data.first, x->data.first))
				x = x->left;
			else
				x = x->right;
		}

		z->parent = y;
		if (y == nullptr)
			root = z;
		else if (defaultCompare(z->data.first, y->data.first))
			y->left = z;
		else
			y->right = z;

		// Fix the red-black tree
		fixInsert(z);
	}

	// Helper function to find a node with a specific key
	Node *findNode(const Key &key)
	{
		Node *current = root;
		while (current != nullptr) {
			if (key == current->data.first)
				return current;
			else if (defaultCompare(key, current->data.first))
				current = current->left;
			else
				current = current->right;
		}
		return nullptr;
	}

	// Helper function to recursively delete the tree
	void deleteTree(Node *node)
	{
		if (node == nullptr)
			return;

		deleteTree(node->left);  // Recursively delete left subtree
		deleteTree(node->right); // Recursively delete right subtree
		delete node;  // Delete the current node
	}

public:
	Map() : root(nullptr) {}

	~Map()
	{
		deleteTree(root);  // Delete the entire tree when the map is destroyed
	}

	// Insert a key-value pair
	void insert(const Key &key, const T &value)
	{
		insert(make_pair< Key, T>(key, value));
	}

	// Find value by key
	T &operator[](const Key &key)
	{
		Node *node = findNode(key);
		if (node) {
			return node->data.second;
		}
		else {
			insert(key, T());
			node = findNode(key);
			return node->data.second;
		}
	}

	// Print the map in-order (for debugging)
	void printInOrder(Node *node)
	{
		if (node == nullptr) return;
		printInOrder(node->left);
		dbg_sprintf(dbgout, "\nPrint: %d : %d", node->data.first, node->data.second);
		printInOrder(node->right);
	}

	void print()
	{
		printInOrder(root);
	}

	// Find a key and return the value
	T *find(const Key &key)
	{
		Node *node = findNode(key);
		if (node != nullptr) {
			return &node->data.second;
		}
		return nullptr;
	}

	bool contains(const Key &key)
	{
		Node *node = findNode(key);
		return node != nullptr;

	}

	bool isEmpty()
	{
		return root == nullptr;
	}
};

