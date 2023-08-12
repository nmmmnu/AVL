#include <cstdint>
#include <cassert>
#include <algorithm>	// max, swap
#include <iostream>

// based on https://medium.com/@mohith.j/balancing-efficiency-exploring-the-avl-trees-7a8ed229515c
// based on https://www.geeksforgeeks.org/deletion-in-an-avl-tree/

namespace avl_impl_{

	using height_t		= uint16_t;
	using signed_height_t	= int32_t;

	template<typename T>
	struct Node{
		T key;

		height_t height	= 1;

		Node *l	= nullptr;
		Node *r	= nullptr;
		Node *p	= nullptr;

		template<typename UT>
		constexpr Node(UT &&key) :
						key(std::forward<UT>(key)){}

		template<typename UT>
		constexpr Node(UT &&key, Node *p) :
						key(std::forward<UT>(key)),
						p(p){}

		constexpr Node(Node &&other) :
					key	(std::move(key		)),
					height	(std::move(height	)),
					l	(std::move(l		)),
					r	(std::move(r		)),
					p	(std::move(p		)){}

		constexpr Node &operator =(Node &&other){
			using std::swap;

			swap(key	, other.key	);
			swap(height	, other.height	);
			swap(l		, other.l	);
			swap(r		, other.r	);
			swap(p		, other.p	);

			return *this;
		}

		void print(bool pretty = false) const{
			std::cout << key << '\n';
		}

		void printPretty(size_t const pad = 0, char const type = ' ') const{
			for(size_t i = 0; i < pad; ++i)
				std::cout << "     ";

			std::cout << "╰──▶ " << key << ' ' << '(' << type << height << ')' << '\n';
		}
	};

	// ----------------------------------------

	template<typename T>
	void print(const Node<T> *node){
		if (!node)
			return;

		print(node->l);
		node->print();
		print(node->r);
	}

	template<typename T>
	void printPretty(const Node<T> *node, size_t const pad = 0, char const type = 'B'){
		if (!node)
			return;

		node->printPretty(pad, type);
		printPretty(node->l, pad + 1, 'L');
		printPretty(node->r, pad + 1, 'R');
	}

	// ----------------------------------------

	template<bool deallocateChildren = true, typename T>
	void deallocate(Node<T> *node){
		if (!node)
			return;

		if constexpr(deallocateChildren){
			deallocate(node->l);
			deallocate(node->r);
		}

		delete node;
	}

	// ----------------------------------------

	template<bool checkNode, typename T>
	constexpr auto *check_(Node<T> *node, const Node<T> *parent = nullptr){
		if constexpr(!checkNode)
			return node;

		if (!node)
			return node;

		assert(node->p == parent);

		auto const balance = getbalance_(node);

		assert(balance >= -1 && balance <= +1);

		check_<checkNode>(node->l, node);
		check_<checkNode>(node->r, node);

		return node;
	}

	// ----------------------------------------

	template<typename T>
	constexpr height_t height_(const Node<T> *node){
		return node ? node->height : 0;
	}

	template<typename T>
	void updateHeight_(Node<T> *node){
		assert(node);

		node->height = std::max(height_(node->l), height_(node->r)) + 1u;
	}

	template<typename T>
	auto *rotateR_(Node<T> *y){
		auto *x = y->l; // guaranteed not null
		auto *t = x->r; // may be null

		/*
		 *    Y      X
		 *   /        \
		 *  X    =>    Y
		 *   \        /
		 *    T      T
		 */

		// Rotate
		x->r = y;
		y->l = t;

		// Fix parents, x and y guaranteed not null
		x->p = y->p;
		y->p = x;
		if (t)
		t->p = y;

		updateHeight_(y); // y is lower than x
		updateHeight_(x);

		return x;
	}

	template<typename T>
	auto *rotateL_(Node<T> *x){
		auto *y = x->r; // guaranteed not null
		auto *t = y->l; // may be null

		/*
		 *  X          Y
		 *   \        /
		 *    Y  =>  X
		 *   /        \
		 *  T          T
		 */

		// Rotate
		y->l = x;
		x->r = t;


		// Fix parents
		y->p = x->p;
		x->p = y;
		if (t)
		t->p = x;

		updateHeight_(x); // x is lower than y
		updateHeight_(y);

		return y;
	}

	template<typename T>
	auto *rotateLR_(Node<T> *x){
		x->l = rotateL_(x->l);
		return rotateR_(x);
	}

	template<typename T>
	auto *rotateRL_(Node<T> *x){
		x->r = rotateR_(x->r);
		return rotateL_(x);
	}

	template<typename T>
	signed_height_t getbalance_(const Node<T> *node){
		auto _ = [](const Node<T> *node){
			return signed_height_t{ height_(node) };
		};

		return node ? _(node->l) - _(node->r) : 0;
	}

	// ----------------------------------------

	template<typename T, typename UT>
	auto *insert_(Node<T> *node, Node<T> *parent, UT &&key, Node<T> * &it){
		using namespace avl_impl_;

		if (!node){
			it = new Node<T>(std::forward<UT>(key), parent);
			return it;
		}

		if (key < node->key){
			node->l = insert_(node->l, node, key, it); // key not forwarded
		}else if (key > node->key){
			node->r = insert_(node->r, node, key, it); // key not forwarded
		}else{
			// Found, not inserted.
			it = node;
			return it;
		}

		updateHeight_(node);

		auto const balance = getbalance_(node);

		if (balance > +1 && key < node->l->key)
			return rotateR_(node);

		if (balance < -1 && key > node->r->key)
			return rotateL_(node);

		if (balance > +1 && key > node->l->key)
			return rotateLR_(node);

		if (balance < -1 && key < node->r->key)
			return rotateRL_(node);

		return node;
	}

	template<bool checkNode, typename T, typename UT>
	const auto *insert(Node<T> * &root, UT &&key){
		constexpr Node<T> *parent = nullptr;

		Node<T> *it;

		root = insert_(root, parent, std::forward<UT>(key), it);

		check_<checkNode>(root);

		return it;
	}

	// ----------------------------------------

	template<typename T>
	auto *minValueNode(Node<T> *node){
		while(node->l)
			node = node->l;

		return node;
	};

	// ----------------------------------------

	template<typename T>
	auto *reBalance_(Node<T> *node){
		assert(node);

		updateHeight_(node);

		int const balance = getbalance_(node);

		if (balance > +1 && getbalance_(node->l) >= 0)
			return rotateR_(node);

		if (balance > +1 && getbalance_(node->l) <  0)
			return rotateLR_(node);

		if (balance < -1 && getbalance_(node->r) <= 0)
			return rotateL_(node);

		if (balance < -1 && getbalance_(node->r) >  0)
			return rotateRL_(node);

		return node;
	}

	template<typename T, typename UT>
	auto *erase_(Node<T> *node, UT &&key, bool &updated){
		if (!node){
			updated = false;
			return node;
		}

		if (key < node->key){
			node->l = erase_(node->l, std::forward<UT>(key), updated);
			return reBalance_(node);
		}

		if (key > node->key){
			node->r = erase_(node->r, std::forward<UT>(key), updated);
			return reBalance_(node);
		}

		// found

		updated = true;

		if (node->l == nullptr || node->r == nullptr){
			// CASE 1: node with no children
			// or
			// CASE 2: node with only one child

			auto *temp = node->l ? node->l : node->r;

			if (!temp){
				// No children case
				temp = node;
				node = nullptr;
			}else{
				// One child case

				// Fix parent
				temp->p = node->p;

				// Move the data
				*node = std::move(*temp);
			}

			// Do not deallocate the children
			deallocate<false>(temp);

			return node ? reBalance_(node) : node;
		} else {
			// CASE 3: node with two children

			// Find min in right subtree
			auto *temp = minValueNode(node->r);

			using std::swap;
			std::swap(node->key, temp->key);

			// remove temp
			bool b;
			node->r = erase_(node->r, temp->key, b);

			return reBalance_(node);
		}
	}

	template<bool checkNode, typename T, typename UT>
	bool erase(Node<T> * &root, UT &&key){
		bool updated;

		root = erase_(root, std::forward<UT>(key), updated);

		check_<checkNode>(root);

		return updated;
	}

	// ----------------------------------------

	template<typename T>
	class iterator{
	public:
		constexpr iterator(const Node<T> *node) : node(node){}

	public:
		using difference_type	= std::ptrdiff_t;
		using value_type	= const T;
		using pointer		= value_type *;
		using reference		= value_type &;
		using iterator_category	= std::forward_iterator_tag;
		// avl tree can support bi-directiona iterator as well

	public:
		iterator &operator++(){
			// left child should be processed.
			// node       should be processed.

			if (node->r){
				// go right
				node = minValueNode(node->r);
				return *this;
			}


			// go up
			while(node->p){
				const auto *copy = node;

				node = node->p;

				if (node->l == copy){
					// we were in left child
					// process the node
					return *this;
				}else{
					// we were in right child
					// go up again
				}
			}

			// we are the root node
			node = nullptr; // std::end()
			return *this;

		}

		reference operator*() const{
			return node->key;
		}

		bool operator==(const iterator &other) const{
			return node == other.node;
		}

		bool operator!=(const iterator &other) const{
			return ! operator==(other);
		}

		pointer operator ->() const{
			return & operator*();
		}

	private:
		const Node<T> *node;
	};

} // namespace avl_impl_



template<typename T, bool checkTree = false>
class AVLTree {
	using Node = avl_impl_::Node<T>;

	Node *root = nullptr;

public:
	using iterator = avl_impl_::iterator<T>;

public:
	constexpr AVLTree() = default;

	~AVLTree(){
		avl_impl_::deallocate(root);
	}

	void clear(){
		avl_impl_::deallocate(root);
		root = nullptr;
	}

public:
	template<bool Exact, typename UT>
	iterator find(UT &&key, std::bool_constant<Exact>) const{
		auto *node = root;

		while(node){
			if (key < node->key){
				if constexpr(!Exact)
					if (node->l == nullptr)
						return findFix__(node, key);

				node = node->l;
				continue;
			}

			if (key > node->key){
				if constexpr(!Exact)
					if (node->r == nullptr)
						return findFix__(node, key);

				node = node->r;
				continue;
			}

			break;
		}

		return node;
	}

	iterator begin() const{
		return avl_impl_::minValueNode(root);
	}

	constexpr static iterator end(){
		return nullptr;
	}

private:
	static iterator findFix__(const Node *node, T const &key){
		while(node)
			if (key > node->key)
				node = node->p;
			else
				break;

		return node;
	}

public:
	template<typename UT>
	iterator insert(UT &&key){
		return avl_impl_::insert<checkTree>(root, key);
	}

	template<typename UT>
	bool erase(UT &&key){
		return avl_impl_::erase<checkTree>(root, key);
	}

public:
	void print() const{
		avl_impl_::print(root);
	}

	void printPretty() const{
		avl_impl_::printPretty(root);
	}
};



int main(){
	auto insert = [](auto &tree, auto const &val){
		auto it = tree.insert(val);
		assert(*it == val);
	};

	if constexpr(true){
		AVLTree<int, true> tree;

		tree.insert(10);
		tree.insert(20);
		tree.insert(30);
		tree.insert(40);
		tree.insert(50);
		tree.insert(60);
		tree.insert(70);
		tree.insert(80);
		tree.insert(90);
		tree.erase(40);
		tree.erase(50);
		tree.erase(60);
		tree.erase(70);

		bool u;

		insert(tree, 3);
		insert(tree, 4);
		insert(tree, 5);
		insert(tree, 6);
		insert(tree, 1);
		insert(tree, 2);

		tree.clear();

		insert(tree, 10);
		u = tree.erase(10); assert(u);

		printf("--------\n"); tree.printPretty();

		tree.clear();

		insert(tree, 10);
		insert(tree, 20);
		insert(tree, 30);
		insert(tree, 40);
		insert(tree, 50);
		insert(tree, 60);
		insert(tree, 70);
		insert(tree, 80);
		insert(tree, 90);

		tree.insert(25);

		printf("--------\n"); tree.printPretty();


		u = tree.erase(25); assert(u);
		u = tree.erase(25); assert(!u);

		printf("--------\n"); tree.printPretty();

		u = tree.erase(40); assert(u);
		u = tree.erase(40); assert(!u);

		printf("--------\n"); tree.printPretty();

		insert(tree, 95);
		insert(tree, 98);

		printf("--------\n"); tree.printPretty();

		for(auto const &x : tree)
			std::cout << x << '\n';

		printf("--------\n");

		assert(tree.find(5, std::true_type{}) == std::end(tree));

		std::cout << *tree.find( 5, std::false_type{}) << '\n';

		assert(*tree.find( 9, std::false_type{}) == 10);
		assert(*tree.find(11, std::false_type{}) == 20);

		assert(*tree.find(49, std::false_type{}) == 50);
		assert(*tree.find(51, std::false_type{}) == 60);

		assert(*tree.find(97, std::false_type{}) == 98);
		assert(tree.find(99, std::false_type{}) == std::end(tree));
	}
}




