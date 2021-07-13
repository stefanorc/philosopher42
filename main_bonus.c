#include "phil_bonus.h"

typedef struct timeval	t_time;
typedef struct timezone	t_zone;

void	wrapped_print(t_opt *opt, t_llint ttime, int pid, char *str)
{
	if (!opt->alive)
		return ;
	pthread_mutex_lock(opt->print_m);
	printf("%lld %d %s\n", ttime, pid, str);
	pthread_mutex_unlock(opt->print_m);
}

t_llint	mtime(void)
{
	t_llint	mtime_res;
	int		res;
	t_time	tv;
	t_zone	tz;

	res = gettimeofday(&tv, &tz);
	mtime_res = tv.tv_sec * 1000;
	mtime_res += tv.tv_usec / 1000;
	return (mtime_res);
}

void	msleep_till(t_llint msec)
{
	t_llint	delta;

	delta = msec - mtime();
	if (delta > 0)
		usleep(delta * 1000);
}

void	msleep(t_llint msec)
{
	usleep(msec * 1000);
}

int	get_forks(t_opt *opt, t_phil *phil)
{
	int	res;

	sem_wait(opt->sem);
	sem_wait(opt->sem);
	return (1);
}

void	drop_forks(t_opt *opt, t_phil *phil)
{
	sem_post(opt->sem);
	sem_post(opt->sem);
}

void	*life(void *arg)
{
	t_opt	*opt;
	t_phil	*phil;
	t_llint	tt;
	int		res;

	phil = (t_phil *)arg;
	opt = phil->opt;
	wrapped_print(opt, mtime(), phil->pid, "start");
	while (opt->alive)
	{
		tt = mtime();
		if (phil->last_eat < tt - opt->time_die)
		{
			opt->alive = 0;
			wrapped_print(opt, mtime(), phil->pid, "died");
			break ;
		}
		if (phil->stage == 0)
		{
			res = get_forks(opt, phil);
			if (res)
			{
				wrapped_print(opt, mtime(), phil->pid, "is eating");
				phil->last_eat = tt;
				phil->stage_time = tt;
				phil->stage = 1;
				phil->fails = 1;
			}
			else
				phil->fails++;
		}
		else if (phil->stage == 1)
		{
			if (phil->stage_time < tt - opt->time_eat)
			{
				wrapped_print(opt, mtime(), phil->pid, "is sleeping");
				drop_forks(opt, phil);
				phil->stage_time = tt;
				phil->stage = 2;
			}
		}
		else if (phil->stage == 2)
		{
			if (phil->stage_time < tt - opt->time_sleep)
			{
				phil->stage_time = tt;
				phil->stage = 0;
				phil->eat_count++;
				if (opt->eat_count >= 0 && phil->eat_count >= opt->eat_count)
				{
					wrapped_print(opt, mtime(), phil->pid, "done!");
					break ;
				}
				else
					wrapped_print(opt, mtime(), phil->pid, "is thinking");
			}
		}
		msleep(1);
	}
	return (0);
}

void	init_mutex(t_opt *opt)
{
	int	i;
	pthread_mutex_t	**fork_m;

	i = 0;
	opt->forks = malloc(sizeof(int *) * (opt->p_count + 1));
	i = 0;
	while (i < opt->p_count)
	{
		opt->forks[i] = malloc(sizeof(int));
		*opt->forks[i] = 0;
		i++;
	}
	opt->forks[i] = opt->forks[0];
	opt->print_m = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(opt->print_m, NULL);
}

int	main(int argc, char **argv)
{
	pthread_t		*tid;
	int				i;
	void			*ret;
	t_opt			*opt;
	t_phil			*phil;
	int				*pids;

	printf("%lld sim started\n", mtime());
	opt = malloc(sizeof(t_opt));
	opt->p_count = atoi(argv[1]);
	opt->time_die = atoi(argv[2]);
	opt->time_eat = atoi(argv[3]);
	opt->time_sleep = atoi(argv[4]);
	if (argc > 5)
		opt->eat_count = atoi(argv[5]);
	else
		opt->eat_count = -1;
	opt->alive = 1;
	char semname[] = "sem5";
	opt->sem = sem_open(semname, O_CREAT, 0644, opt->p_count);
	printf("sem create %p\n", opt->sem);
	init_mutex(opt);
	pids = malloc(sizeof(int) * opt->p_count);
	tid = malloc(sizeof(pthread_t) * opt->p_count);
	i = 0;
	while (i < opt->p_count)
	{
		msleep(100);
		phil = malloc(sizeof(t_phil));
		phil->stage = 0;
		phil->fails = 1;
		phil->eat_count = 0;
		phil->stage_time = 0;
		phil->last_eat = mtime();
		phil->opt = opt;
		phil->pid = i + 1;
		pids[i] = fork();
		if (!pids[i])
		{
			life(phil);
			printf("child %d done\n", phil->pid);
			exit(0);
		}
		i++;
	}
	i = 0;
	while (i < opt->p_count)
	{
		printf("pid %d wait\n", pids[i]);
		waitpid(pids[i], &pids[i], 0);
		i++;
	}
	return (0);
}