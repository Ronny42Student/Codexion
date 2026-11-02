/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap_utils.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nrajaoar <nrajaoar@student.42antananari    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/11/02 16:23:46 by nrajaoar          #+#    #+#             */
/*   Updated: 2026/11/02 18:00:00 by nrajaoar         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	pq_peek_coder(t_pq *pq)
{
	if (pq->size == 0)
		return (-1);
	return (pq->heap[0].coder_id);
}

void	sift_up(t_pq *pq, int i, int is_edf)
{
	int		parent;
	t_req	tmp;

	while (i > 0)
	{
		parent = (i - 1) / 2;
		if (!is_higher(pq->heap[i], pq->heap[parent], is_edf))
			break ;
		tmp = pq->heap[i];
		pq->heap[i] = pq->heap[parent];
		pq->heap[parent] = tmp;
		i = parent;
	}
}

void	sift_down(t_pq *pq, int i, int is_edf)
{
	int		l;
	int		r;
	int		s;
	t_req	tmp;

	while (1)
	{
		l = 2 * i + 1;
		r = 2 * i + 2;
		s = i;
		if (l < pq->size && is_higher(pq->heap[l], pq->heap[s], is_edf))
			s = l;
		if (r < pq->size && is_higher(pq->heap[r], pq->heap[s], is_edf))
			s = r;
		if (s == i)
			break ;
		tmp = pq->heap[i];
		pq->heap[i] = pq->heap[s];
		pq->heap[s] = tmp;
		i = s;
	}
}

void	pq_remove(t_pq *pq, int coder_id, int is_edf)
{
	int	i;

	i = 0;
	while (i < pq->size)
	{
		if (pq->heap[i].coder_id == coder_id)
		{
			pq->size--;
			if (i < pq->size)
			{
				pq->heap[i] = pq->heap[pq->size];
				sift_up(pq, i, is_edf);
				sift_down(pq, i, is_edf);
			}
			return ;
		}
		i++;
	}
}
