/*
 * Stack implementation
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

#include "../include/stack.h"

/*
 * Create a new Stack
 * Alocate a head node
 */
d_stack_p get_stack(void) {
	d_stack_p stack = calloc(1, sizeof(d_stack_t));

	stack->head = calloc(1, sizeof(dlist_pp));
	return stack;
}

/*
 * Push a value to Stack
 * Allocates a new dlist node and inserts value
 */
bool push(d_stack_p stack, void *val) {
	dlist_p nodeptr = calloc(1, sizeof(dlist_t));

	nodeptr->data = val;

	if (add_head_dlist(stack->head, nodeptr)) /* Last In */
		return TRUE;
	else
		return FALSE;
}

/*
 * Pop a value from Stack
 * Deallocates the dlist node holding the value
 */
void *pop(d_stack_p stack) {
	void *data = get_head_dlist(stack->head); /* First out */

	if (delete_head_dlist(stack->head) == -1)
        dprintf(fdOut, "head or first node is NULL!\n");

	return data;
}

/*
 * Check if a stack has any elements
 */
bool isStackEmpty(d_stack_p stack) {
	if (get_head_dlist(stack->head))
		return TRUE;

	return FALSE;
}

/*
 * Deallocate all dlist nodes
 * Destroy Stack
 */
bool destroy_stack(d_stack_p stack) {
	if (destroy_dlist(stack->head) == -1)
		return FALSE;

	free(stack->head);
	free(stack);

	return TRUE;
}
