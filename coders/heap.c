/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrajaoar <nrajaoar@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 16:23:25 by nrajaoar          #+#    #+#             */
/*   Updated: 2026/11/02 16:23:26 by nrajaoar         ###   ########.fr       */
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
	int		i;
	int		parent;
	t_req	tmp;

	if (pq->size >= pq->capacity)
		return (0);
	i = pq->size;
	pq->heap[i] = req;
	pq->size++;
	while (i > 0)
	{
		parent = (i - 1) / 2;
		if (is_higher(pq->heap[i], pq->heap[parent], is_edf))
		{
			tmp = pq->heap[i];
			pq->heap[i] = pq->heap[parent];
			pq->heap[parent] = tmp;
			i = parent;
		}
		else
			break ;
	}
	return (1);
}

t_req	pq_pop(t_pq *pq, int is_edf)
{
	t_req	top;
	int		i;
	int		l;
	int		r;
	int		s;
	t_req	tmp;

	top = pq->heap[0];
	pq->size--;
	pq->heap[0] = pq->heap[pq->size];
	i = 0;
	while (1)
	{
		l = 2 * i + 1;
		r = 2 * i + 2;
		s = i;
		if (l < pq->size && is_higher(pq->heap[l], pq->heap[s], is_edf))
			s = l;
		if (r < pq->size && is_higher(pq->heap[r], pq->heap[s], is_edf))
			s = r;
		if (s != i)
		{
			tmp = pq->heap[i];
			pq->heap[i] = pq->heap[s];
			pq->heap[s] = tmp;
			i = s;
		}
		else
			break ;
	}
	return (top);
}
