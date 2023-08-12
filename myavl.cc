#include <cstdint>
#include <cassert>
#include <algorithm>	// max
#include <functional>	// function for testing

#include <iostream>



namespace alv_impl_{

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

			std::cout << "╰──▶ " << data << ' ' << '(' << type << int16_t{balance} << ')' << '\n';
		}
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



	template<typename T>
	constexpr balance_t balance_(const Node<T> *node){
		return node ? node->balance : 0;
	}



} // namespace alv_impl_


template<typename T>
class AVLTree{
	using Node = alv_impl_::Node<T>;

	Node *root = nullptr;

private:
	template<typename UT>
	static Node *createNode__(UT &&data, Node *parent){
		return new Node(std::forward<UT>(data), parent);
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
					node->balance    = 0;
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
					node->balance    = 0;
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
						node->r->balance = -1;

					rotateLR_(node);
				}

				break;
			}

			if (!node->p)
				break;

			if (node->p->l == node)
				--node->p->balance;
			else
				++node->p->balance;

			node = node->p;
		}
	}

public:
	void printPretty() const{
		return alv_impl_::printPretty(root);
	}

	void check() const{
		return alv_impl_::check(root);
	}

public:
	template<typename UT>
	void insert(UT &&data){
		if (!root){
			// tree is empty.
			// insert new node, no balance.

			root = createNode__(std::forward<UT>(data), nullptr);

			return;
		}

		Node *node   = root;

		while(true){
			if (data < node->data){
				if (!node->l){
					node->l = createNode__(std::forward<UT>(data), node);
					--node->balance;
					rebalanceAfterInsert_(node);
					return;
				}else{
					node = node->l;
					continue;
				}
			}

			if (data > node->data){
				if (!node->r){
					node->r = createNode__(std::forward<UT>(data), node);
					++node->balance;
					rebalanceAfterInsert_(node);
					return;
				}else{
					node = node->r;
					continue;
				}
			}

			if (data == node->data){
				// found, not insert, not balance.
				return;
			}
		}

		// never reach here.
	}

};



int main(){
	AVLTree<int> tree;

	for(int i = 0; i < 1024 * 16; ++i){
		tree.insert(i);
		tree.check();
	}

	tree.printPretty();
}


