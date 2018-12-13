/*
 * Binary Search Tree abstraction
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

#include "common.h"

#pragma once

typedef struct tree{
	int data;
	struct tree *left;
	struct tree *right;
} tree_t, *tree_p, **tree_pp;

/* Delete a node from tree */
bool delete_tree_node (tree_pp head, int val);

/* Print a tree in preorder */
int print_tree (tree_p root);
