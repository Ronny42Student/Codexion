#include "codexion.h"

int	pq_peek_coder(t_pq *pq)
{
	if (pq->size == 0)
		return (-1);
	return (pq->heap[0].coder_id);
}

static void	sift_down(t_pq *pq, int i, int is_edf)
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
			pq->heap[i] = pq->heap[pq->size];
			sift_down(pq, i, is_edf);
			return ;
		}
		i++;
	}
}
