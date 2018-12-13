/*
 * AVL Tree abstraction
 * Ref: https://en.wikipedia.org/wiki/AVL_tree
 *
 * Author: Arun Prakash Jana <engineerarun@gmail.com>
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

#pragma once


#include "common.h"
#include "stack.h"
#include "queue.h"


typedef struct avl{
	int keyNode;
	int data;
	struct avl *left;
	struct avl *right;
	int height;
} avl_t, *avl_p, **avl_pp;


/* Initialize an AVL tree */
avl_pp init_avl (void);

/* Insert a node in AVL tree */
bool insert_avl_node (avl_pp head, int key, int data);

/* Delete a node from AVL tree */
bool delete_avl_node (avl_pp head, int key);

/* Destroy the tree */
int destroy_avl (avl_pp head);

/* Print a tree in preorder */
int print_avl (avl_p root, avl_p parent);
