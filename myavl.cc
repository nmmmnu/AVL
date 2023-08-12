#include <cstdint>
#include <cassert>
#include <algorithm>	// max, swap

#include <iostream>



namespace avl_impl_{

	using balance_t        = int8_t;



	template<typename T>
	struct Node{
		T data;

		balance_t balance = 0;

		Node *l	= nullptr;
		Node *r	= nullptr;
		Node *p	= nullptr;

		template<typename UT>
		constexpr Node(UT &&data) :
						data(std::forward<UT>(data)){}

		template<typename UT>
		constexpr Node(UT &&data, Node *p) :
						data(std::forward<UT>(data)),
						p(p){}

		constexpr Node(Node &&other) :
					data	(std::move(data		)),
					balance	(std::move(balance	)),
					l	(std::move(l		)),
					r	(std::move(r		)),
					p	(std::move(p		)){}

		constexpr Node &operator =(Node &&other){
			using std::swap;

			swap(data	, other.data	);
			swap(balance	, other.balance	);
			swap(l		, other.l	);
			swap(r		, other.r	);
			swap(p		, other.p	);

			return *this;
		}

		void printPretty(size_t const pad = 0, char const type = ' ') const{
			for(size_t i = 0; i < pad; ++i)
				std::cout << "     ";

			if constexpr(0)
				std::cout << "╰──▶ " << data << ' ' << '(' << type << int16_t{balance} << ')' << '\n';
			else
				std::cout << "╰──▶ " << data << ' ' << '(' << type << int16_t{balance} << ',' << ((void *) this) << ')' << '\n';
		}
	};



	template<typename T>
	Node<T> *minValueNode(Node<T> *node){
		if (!node)
			return nullptr;

		while(node->l)
			node = node->l;

		return node;
	}

	template<typename T>
	const Node<T> *minValueNode(const Node<T> *node){
		if (!node)
			return nullptr;

		while(node->l)
			node = node->l;

		return node;
	}



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
			return node->data;
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



	template<typename T>
	void printPretty(const Node<T> *node, size_t const pad = 0, char const type = 'B'){
		// not important, so it stay recursive.

		if (!node)
			return;

		node->printPretty(pad, type);
		printPretty(node->l, pad + 1, 'L');
		printPretty(node->r, pad + 1, 'R');
	}



	template<typename T, bool CheckHeight = false>
	void check(const Node<T> *node, const Node<T> *parent = nullptr){
		// not important, so it stay recursive.

		if (!node)
			return;

		assert(node->p == parent);
		assert(node->balance >= -1 && node->balance <= +1);

		if constexpr(CheckHeight){
			auto height = [](const Node<T> *node) -> int{
				auto _ = [](const auto *node, auto _) -> int{
					if (!node)
						return 0;

					return std::max(
						_(node->l, _),
						_(node->r, _)
					) + 1;
				};

				return _(node, _);
			};

			auto const balance = height(node->r) - height(node->l);
			assert(balance >= -1 && balance <= +1);
			assert(balance == node->balance);
		}

		if (node->l)
			assert(node->l->data < node->data);

		if (node->r)
			assert(node->r->data > node->data);

		check(node->l, node);
		check(node->r, node);
	}



} // namespace alv_impl_


template<typename T>
class AVLTree{
	using Node = typename avl_impl_::Node<T>;

	Node *root = nullptr;

public:
	~AVLTree(){
		deallocateTree__(root);
	}

public:
	using iterator = avl_impl_::iterator<T>;

public:
	void printPretty() const{
		return avl_impl_::printPretty(root);
	}

	void check() const{
		return avl_impl_::check(root);
	}

public:
	void clear(){
		deallocateTree__(root);
		root = nullptr;
	}

	template<typename UT>
	iterator insert(UT &&data){
		if (!root){
			// tree is empty.
			// insert, no balance.

			root = allocateNode__(std::forward<UT>(data), nullptr);

			return root;
		}

		Node *node   = root;

		while(true){
			if (data < node->data){
				if (!node->l){
					auto *new_node = allocateNode__(std::forward<UT>(data), node);
					node->l = new_node;
					--node->balance;
					rebalanceAfterInsert_(node);
					return new_node;
				}else{
					node = node->l;
					continue;
				}
			}

			if (data > node->data){
				if (!node->r){
					auto *new_node = allocateNode__(std::forward<UT>(data), node);
					node->r = new_node;
					++node->balance;
					rebalanceAfterInsert_(node);
					return new_node;
				}else{
					node = node->r;
					continue;
				}
			}

			if (data == node->data){
				// found, not insert, no balance.
				return end();
			}
		}

		// never reach here.
	}

	template<typename UT>
	bool erase(UT const &key){
		auto *node = root;

		while(node){
			if (key < node->data){
				node = node->l;
				continue;
			}

			if (key > node->data){
				node = node->r;
				continue;
			}

			break;
		}

		if (!node)
			return false;

		if (node->l && node->r){
			// CASE 3 - node two children
			using namespace avl_impl_;
			auto *successor = minValueNode(node->r);

			using std::swap;
			swap(node->data, successor->data);

			node = successor;
		}

		if (auto *child = node->l ? node->l : node->r; child){
			// CASE 2: node with only one child
			child->p = node->p;

			if (!node->p){
				deallocateNode__(node);
				this->root = child;
				return true;
			}

			if (auto *parent = node->p; node == parent->l){
				parent->l = child;
				++parent->balance;

				deallocateNode__(node);

				if (parent->balance == +1){
					return true;
				}else{
					rebalanceAfterErase_(parent);
					return true;
				}
			}else{ // node == parent->r
				parent->r = child;
				--parent->balance;

				deallocateNode__(node);

				if (parent->balance == -1){
					return true;
				}else{
					rebalanceAfterErase_(parent);
					return true;
				}
			}
		}

		// CASE 1: node with no children

		if (!node->p){
			deallocateNode__(node);
			this->root = nullptr;
			return true;
		}

		if (auto *parent = node->p; node == parent->l){
			parent->l = nullptr;
			++parent->balance;

			deallocateNode__(node);

			if (parent->balance == +1){
				return true;
			}else{
				rebalanceAfterErase_(parent);
				return true;
			}
		}else{ // node == parent->r
			parent->r = nullptr;
			--parent->balance;

			deallocateNode__(node);

			if (parent->balance == -1){
				return true;
			}else{
				rebalanceAfterErase_(parent);
				return true;
			}
		}
	}

public:
	template<bool Exact, typename UT>
	iterator find(UT const &key, std::bool_constant<Exact>) const{
		auto *node = root;

		while(node){
			if (key < node->data){
				if constexpr(!Exact)
					if (node->l == nullptr)
						return findFix__(node, key);

				node = node->l;
				continue;
			}

			if (key > node->data){
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
	template<typename UT>
	static iterator findFix__(const Node *node, UT const &key){
		while(node)
			if (key > node->data)
				node = node->p;
			else
				break;

		return node;
	}

	template<typename UT>
	static Node *allocateNode__(UT &&data, Node *parent){
		return new Node(std::forward<UT>(data), parent);
	}

	static void deallocateNode__(Node *node){
		assert(node);
		delete node;
	}

	void rotateL_(Node *n){
		/*
		 *     n             r
		 *      \           /
		 *       r   ==>   n
		 *      /           \
		 *     t             t
		 */

		auto *r = n->r;
		auto *t = r->l;
		n->r = t;

		if (t)
			t->p = n;

		r->p = n->p;

		if (!n->p)
			this->root = r;
		else if (n->p->l == n)
			n->p->l = r;
		else
			n->p->r = r;

		r->l = n;
		n->p = r;
	}

	void rotateR_(Node *n){
		/*
		 *     n             l
		 *    /               \
		 *   l       ==>       n
		 *    \               /
		 *     t             t
		 */

		auto *l = n->l;
		auto *t = l->r;
		n->l = t;

		if (t)
			t->p = n;

		l->p = n->p;

		if (!n->p)
			this->root = l;
		else if (n->p->r == n)
			n->p->r = l;
		else
			n->p->l = l;

		l->r = n;
		n->p = l;
	}

	void rotateRL_(Node *node){
		rotateR_(node->r);
		rotateL_(node);
	}

	void rotateLR_(Node *node){
		rotateL_(node->l);
		rotateR_(node);
	}

	void rebalanceAfterInsert_(Node *node){
		while(node->balance){
			if (node->balance == +2){
				// right heavy
				if (node->r->balance == +1){
					node->balance = 0;
					node->r->balance = 0;

					rotateL_(node);
				}else{ // node->r->balance == -1
					auto const rlBalance = node->r->l->balance;

					node->r->l->balance = 0;
					node->r->balance = 0;
					node->balance = 0;

					if (rlBalance == +1)
						node->balance = -1;
					else if (rlBalance == -1)
						node->r->balance = +1;

					rotateRL_(node);
				}

				break;
			}

			if (node->balance == -2){
				// left heavy
				if (node->l->balance == -1){
					node->balance = 0;
					node->l->balance = 0;

					rotateR_(node);
				}else{ // node->r->balance == +1
					auto const lrBalance = node->l->r->balance;

					node->l->r->balance = 0;
					node->l->balance = 0;
					node->balance = 0;

					if (lrBalance == -1)
						node->balance = +1;
					else if (lrBalance == +1)
						node->l->balance = -1;

					rotateLR_(node);
				}

				break;
			}

			auto *parent = node->p;

			if (!parent)
				return;

			if (parent->l == node)
				--parent->balance;
			else
				++parent->balance;

			node = node->p;
		}
	}

	void rebalanceAfterErase_(Node *node){
		assert(node);

		while(true){
			if (node->balance == +2){
				// right heavy

				if (node->r->balance == +1){
					node->balance = 0;
					node->r->balance = 0;

					rotateL_(node);
				}else if(node->r->balance == 0){
					node->balance = +1;
					node->r->balance = -1;

					rotateL_(node);
				}else{ // node->r->balance == -1
					auto const rlBalance = node->r->l->balance;

					node->r->l->balance = 0;
					node->r->balance = 0;
					node->balance = 0;

					if (rlBalance == +1)
						node->balance = -1;
					else if (rlBalance == -1)
						node->r->balance = +1;

					rotateRL_(node);
				}

				node = node->p;
			}else
			if (node->balance == -2){
				// left heavy

				if (node->l->balance == -1){
					node->balance = 0;
					node->r->balance = 0;

					rotateR_(node);
				}else if(node->l->balance == 0){
					node->balance = -1;
					node->l->balance = +1;

					rotateR_(node);
				}else{ // node->l->balance == +1
					auto const lrBalance = node->l->r->balance;

					node->l->r->balance = 0;
					node->l->balance = 0;
					node->balance = 0;

					if (lrBalance == -1)
						node->balance = 1;
					else if (lrBalance == +1)
						node->l->balance = -1;

					rotateLR_(node);
				}

				node = node->p;
			}

			auto *parent = node->p;

			if (!parent)
				return;

			if (node == parent->l){
				++parent->balance;

				if (parent->balance == +1)
					return;
			}else{ // node == parent->r
				--parent->balance;

				if (parent->balance == -1)
					return;
			}

			node = node->p;
		}
	}

private:
	static void deallocateTree__(Node *node){
		// seems there is no viable iterative alternative
		if (!node)
			return;

		deallocateTree__(node->l);
		deallocateTree__(node->r);
		deallocateNode__(node);
	}

};


#include <ctime>

int main(){
	auto insert = [](auto &tree, auto const &val){
		auto it = tree.insert(val);
		assert(*it == val);
		tree.check();
	};

	if constexpr(true){
		AVLTree<int> tree;
		tree.insert( 167958);
		tree.insert( 480816);
		tree.insert( 587341);
		tree.insert( 753184);
		tree.insert( 577370);
		tree.insert( 473704);
		tree.insert( 355773);
		tree.insert( 128659);
		tree.insert( 517515);
		tree.insert( 753334);
		tree.insert(1036150);
		tree.insert( 751291);
		tree.insert( 544425);
		tree.insert( 728099);
		tree.insert( 644954);
		tree.insert( 521484);
		tree.insert(  47207);
		tree.insert( 688599);
		tree.insert( 262359);
		tree.insert(   8547);
		tree.insert( 482991);
		tree.insert( 571029);
		tree.insert(  45309);
		tree.insert( 615399);
		tree.insert( 348695);
		tree.insert( 956852);
	//	tree.printPretty();
		tree.check();
		tree.insert( 403837);

		tree.clear();

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

		u = tree.erase(80); assert(u);
		u = tree.erase(80); assert(!u);

		printf("--------\n"); tree.printPretty();

		insert(tree, 95);
		insert(tree, 98);

		printf("--------\n"); tree.printPretty();

		for(auto const &x : tree)
			std::cout << x << '\n';

		printf("--------\n");

		assert( tree.find( 5, std::true_type{}) == std::end(tree));
		assert(*tree.find( 5, std::false_type{}) == 10);
		assert(*tree.find( 9, std::false_type{}) == 10);
		assert(*tree.find(11, std::false_type{}) == 20);

		assert(*tree.find(49, std::false_type{}) == 50);
		assert(*tree.find(51, std::false_type{}) == 60);

		assert(*tree.find(97, std::false_type{}) == 98);
		assert( tree.find(99, std::false_type{}) == std::end(tree));
	}else{
		AVLTree<int> tree;

		srand(time(0));

		for(int i = 0; i < 1024; ++i){
			int x = rand() % (1024 * 1024);
			printf("%4d | %d\n", i, x);
			tree.insert(x);
		}

		tree.check();
	}
}
