/*
 * Iterative Binary Search Tree implementation
 *
 * Author: Ananya Jana <ananya.jana@gmail.com>
 * Copyright (C) 2015 by Arun Prakash Jana <engineerarun@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with dslib.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../include/queue.h"
#include "../include/tree.h"

/*
 * Delete all nodes of a tree
 */
int delete_tree_nodes (tree_p root){
	int count = 0;

	if (!root){
		dprintf (STDERR_FILENO, "root invalid.\n");
		return -1;
	}

	if (root->left)
		count += delete_tree_nodes (root->left);

	if (root->right)
		count += delete_tree_nodes (root->right);

	free (root);
	root = NULL;

	return ++count;
}

/*=======================================================*/
/*            Library exposed APIs start here            */
/*=======================================================*/

/*
 * Delete a node from tree
 */
bool delete_tree_node (tree_pp head, int val){
	tree_p root = NULL;
	tree_p prev = NULL;
	int direction;

	if (!head){
		dprintf (STDERR_FILENO, "Initialize tree first.\n");
		return FALSE;
	}

	root = *head;

	while (root){
		if (val < root->data){
			if (!root->left)
				break;

			prev = root;
			direction = LEFT;
			root = root->left;
		}
		else if (val > root->data){ /* Greater elements are in right subtree */
			if (!root->right)
				break;

			prev = root;
			direction = RIGHT;
			root = root->right;
		}
		else{ /* Match found */
			if (!root->left){
				if (prev){
					if (direction == LEFT)
						prev->left = root->right;
					else
						prev->right = root->right;
				}
				else /* This was the root node */
					*head = root->right;

				free (root);
				return TRUE;
			}
			else if (!root->right){
				if (prev){
					if (direction == LEFT)
						prev->left = root->left;
					else
						prev->right = root->left;
				}
				else /* This was the root node */
					*head = root->left;

				free (root);
				return TRUE;
			}
			else{ /* Both subtrees have children */
				/* Delete inorder successor */
				tree_p min = root->right;
				while (min->left)
					min = min->left;

				root->data = min->data;
				/* Let's use some recursion here */
				delete_tree_node (&(root->right), min->data);

				return TRUE;
			}
		}
	}

	/* Fall through if root is NULL */
	return FALSE;
}

/*
 * Print the values in a tree in preorder
 */
int print_tree (tree_p root){
	int count = 0;

	if (!root){
		dprintf (STDERR_FILENO, "root invalid.\n");
		return -1;
	}

	log(DEBUG, "keyNode: %d.\n", root->data);
	++count;

	if (root->left)
		count += print_tree (root->left);
	if (root->right)
		count += print_tree (root->right);

	return count;
}
