/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrajaoar <nrajaoar@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 16:23:46 by nrajaoar          #+#    #+#             */
/*   Updated: 2026/11/02 18:00:00 by nrajaoar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	pq_init(t_pq *pq, int capacity)
{
	pq->heap = malloc(sizeof(t_req) * capacity);
	if (!pq->heap)
		return (0);
	pq->capacity = capacity;
	pq->size = 0;
	return (1);
}

void	pq_destroy(t_pq *pq)
{
	if (pq->heap)
	{
		free(pq->heap);
		pq->heap = NULL;
	}
	pq->size = 0;
	pq->capacity = 0;
}

int	is_higher(t_req a, t_req b, int is_edf)
{
	if (is_edf)
	{
		if (a.deadline != b.deadline)
			return (a.deadline < b.deadline);
		if (a.req_seq != b.req_seq)
			return (a.req_seq < b.req_seq);
		return (a.coder_id < b.coder_id);
	}
	if (a.req_seq != b.req_seq)
		return (a.req_seq < b.req_seq);
	return (a.coder_id < b.coder_id);
}

int	pq_push(t_pq *pq, t_req req, int is_edf)
{
	if (pq->size >= pq->capacity)
		return (0);
	pq->heap[pq->size] = req;
	pq->size++;
	sift_up(pq, pq->size - 1, is_edf);
	return (1);
}

t_req	pq_pop(t_pq *pq, int is_edf)
{
	t_req	top;

	top = pq->heap[0];
	pq->size--;
	pq->heap[0] = pq->heap[pq->size];
	sift_down(pq, 0, is_edf);
	return (top);
}
