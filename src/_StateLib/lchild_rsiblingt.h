#ifndef LCHILD_RSIBLINGT_H
#define LCHILD_RSIBLINGT_H
#include <stdio.h>
using namespace std;
#include "../ExprLib/mod_vars.h"
//class shared_state;
class lchild_rsiblingt {
public:
	shared_state* val;
	lchild_rsiblingt *child;
	lchild_rsiblingt *sibling;
public:

	lchild_rsiblingt() {
		val = NULL;
		this->child = NULL;
		this->sibling = NULL;
	}
	lchild_rsiblingt(shared_state* P) {
		val = P;
		this->child = NULL;
		this->sibling = NULL;
	}
	// Creating new Node
	lchild_rsiblingt* newNode(shared_state* data) {
		lchild_rsiblingt *newNode = new lchild_rsiblingt();
		newNode->sibling = newNode->child = NULL;
		newNode->val = data;
		return newNode;
	}

	// Adds a sibling to a list with starting with n
	lchild_rsiblingt *addSibling(lchild_rsiblingt *n, shared_state* data) {
		if (n == NULL)
			return NULL;

		while (n->sibling)
			n = n->sibling;

		return (n->sibling = newNode(data));
	}

	// Add child Node to a Node
	lchild_rsiblingt *addtothisChild(lchild_rsiblingt * n, shared_state* data) {
		if (n == NULL)
			return NULL;

		// Check if child list is not empty.
		if (n->child)
			return addSibling(n->child, data);
		else
			return (n->child = newNode(data));
	}

	lchild_rsiblingt * AddChild(lchild_rsiblingt * newchild_) {
		/*  lchild_rsiblingt* addedChild=NULL;
		 addedChild->val=newchild_;*/

		if (this->child == NULL) {
			this->child = newchild_;
		} else {
			lchild_rsiblingt* node = this->child;
			while (node->sibling) {
				node = node->sibling;
			}
			node->sibling = newchild_;
		}
		return newchild_;
	}
	int traverseTree(lchild_rsiblingt* root) {
		if (root == NULL)
			return 0;

		while (root) {
			if (root->child)
				return 1 + traverseTree(root->child);
			root = root->sibling;
		}
	}

};
#endif
