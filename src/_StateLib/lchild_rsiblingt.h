#ifndef LCHILD_RSIBLINGT_H
#define LCHILD_RSIBLINGT_H
#include <stdio.h>
#include <string.h>
#include <set>
#include "statelib.h"
#include "../ExprLib/mod_vars.h"
#include "../include/splay.h"
//class shared_state;
class lchild_rsiblingt {
public:
	shared_state* val;
	lchild_rsiblingt *child;
	lchild_rsiblingt *sibling;

//private:
	std::set<shared_state*>SetOfState;
	List<shared_state> ListOfState;
	//SplayOfPointers <shared_state> p;
public:
	lchild_rsiblingt() {
		val = NULL;
		this->child = NULL;
		this->sibling = NULL;
	}

	lchild_rsiblingt(shared_state* P) {
		//this->val->fillFrom(P);
		val = P;
		this->child = NULL;
		this->sibling = NULL;
	}
	// Creating new Node
	lchild_rsiblingt* newNode(shared_state* data) {
		lchild_rsiblingt *newNode = new lchild_rsiblingt();
		newNode->sibling = newNode->child = NULL;
		//newNode->val->fillFrom(data);
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
		//PrintSharedState(n->val);
		//PrintSharedState(data);
		// Check if child list is not empty.
		//count++;
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
    // keep compiler happy
    return 0;
	}
	int getNumEdges(lchild_rsiblingt* root) {
		if (root == NULL)
			return 0;

		while (root) {
			if (root->child)
				return 1 + getNumEdges(root->child);
			root = root->sibling;
		}
    // keep compiler happy
    return 0;
	}

	int getNumState(lchild_rsiblingt* root,bool writeState) {
		if (root == NULL)
			return 0;

		while (root) {
			//SetOfState.insert(root->val);
			if(SetOfState.insert(root->val).second){
				if(writeState)
				{
					PrintSharedState(root->val);
				}
			}
			//p.Insert(root->val);
			if (root->child) {

				//ListOfState.Append(root->child->val);
				if(SetOfState.insert(root->child->val).second){
					if(writeState)
					{
						PrintSharedState(root->child->val);
					}
				}
				getNumState(root->child,writeState);

			}
			lchild_rsiblingt* nextroot = root->sibling;
			if (root != NULL) {
				if (nextroot != NULL) {
					//ListOfState.Append(nextroot->val);
					if(SetOfState.insert(nextroot->val).second){
						if(writeState)
						{
							PrintSharedState(nextroot->val);
						}
					}
				}
				root = nextroot;
			}
		}
		return SetOfState.size();//ListOfState.Length();
	}
	void showArcsTree(lchild_rsiblingt* root) {
		if (root == NULL)
			return;

		while (root) {
			if (root->child) {
				//PrintSharedState(root->val);
				PrintState(root);
				printf("To");
				PrintState(root->child);
				//cout << root->data << "TOO" << root->child->data;
				showArcsTree(root->child);

			}
			lchild_rsiblingt* nextroot = root->sibling;
			if (root != NULL) {
				if (nextroot != NULL) {
					PrintState(root);
					printf("TO");
					PrintState(nextroot);
				}
				//cout << " " << root->data << "TO" << nextroot->data;
				root = nextroot;
			}
		}

	}
	void show(OutputStream &s, shared_state* curr_st) {
		s << " state ";
		curr_st->Print(s, 0);
	}
	void showArcsTreewithOS(lchild_rsiblingt* root, OutputStream &s) {
		if (root == NULL)
			return;

		while (root) {
			if (root->child) {
				root->val->Print(s, 2);
				printf("To");
				root->child->val->Print(s, 2);
				showArcsTree(root->child);
			}
			lchild_rsiblingt* nextroot = root->sibling;
			if (root != NULL) {
				if (nextroot != NULL) {
					root->val->Print(s, 2);
					printf("TO");
					nextroot->val->Print(s, 2);
				}
				root = nextroot;
			}
		}

	}
	void PrintState(lchild_rsiblingt* root) {
		printf("\n[ %d", root->val->get(0));
		for (int n = 1; n < root->val->getNumStateVars(); n++)
			printf(", %d", root->val->get(n));
		printf("]\n");
	}
	void PrintSharedState(shared_state* root) {
		printf("***[ %d", root->get(0));
		for (int n = 1; n < root->getNumStateVars(); n++)
			printf(", %d", root->get(n));
		printf("]\n");
	}
};
#endif
