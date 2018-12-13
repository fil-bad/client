/*
 * Circular Doubly Linked List implementation
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

#include "../include/dlist.h"


/*
 * Add a node to the head of dlist
 */
int add_head_dlist (dlist_pp head, dlist_p node){
	if (!node){    //nodo non nullo
		dprintf (STDERR_FILENO, "[dlist]node is NULL!\n");
		return -1;
	}

	if (!head){    //puntatore del puntatore al nodo testa non nullo
		dprintf (STDERR_FILENO, "[dlist]head is NULL!\n");
		return -1;
	}

	if (!*head){   //se il puntatote ai nodi è 0 allora nodo diventa la testa e si auto referenzia
		node->next = node;
		node->prev = node;
		*head = node;

		return 0;
	}
	//nodo viene messo in testa e head slitta
	node->next = *head; /* Current head become head->next */
	node->prev = (*head)->prev;
	(*head)->prev->next = node;
	(*head)->prev = node;
	*head = node;

	return 0;
}

/*
 * Get the value in the head node of dlist
 * The node is not deleted
 */
void *get_head_dlist (dlist_pp head){
	if (!head || !*head){
		dprintf (STDERR_FILENO, "[dlist]head or first node is NULL!\n");
		return NULL;
	}

	return (*head)->data;
}

/*
 * Delete the head node of dlist
 */
int delete_head_dlist (dlist_pp head){
	dlist_p tmp;

	if (!head || !*head){
		dprintf (STDERR_FILENO, "[dlist]No nodes to delete!\n");
		return -1;
	}

	if (*head == (*head)->next){   //elimina se stesso
		free (*head);
		*head = NULL;
		return 0;
	}

	tmp = *head;
	free (tmp->data);
	(*head)->data = NULL;
	(*head)->prev->next = (*head)->next;
	(*head)->next->prev = (*head)->prev;
	*head = (*head)->next; /* head->next becomes next head */

	free (tmp);

	return 0;
}

/*
 * Deallocate all memory and destroy the dlist
 * Returns the number of nodes deleted
 */
int destroy_dlist (dlist_pp head){
	dlist_p tmp;
	int deleted = 0;

	if (!head || !*head){
		dprintf (STDERR_FILENO, "[dlist]No nodes to delete.\n");
		return -1;
	}

	/* Set tail->next to NULL to end deletion loop */
	(*head)->prev->next = NULL;

	while (*head){
		tmp = *head;
		free (tmp->data);
		(*head)->data = NULL;
		(*head)->prev = NULL;
		*head = (*head)->next;

		free (tmp);
		deleted++;
	}
	//alla fine head dovrebbe tornare Null
	return deleted;
}
