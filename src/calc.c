int calc_percentage(int a, int b) {
	return (a * 100) / b;
}

int calc_u_to_ms(long long usec) {
	return usec / 1000;
}

int calc_u_to_msdec(long long usec) {
	return usec % 1000;
}

// https://stackoverflow.com/a/1624646
static long long ft_sqrt(long long a){
    int i;
    unsigned long rem = 0;
    unsigned long root = 0;
    for (i = 0; i < 16; i++){
        root <<= 1;
        rem = (rem << 2) | (a >> 30);
        a <<= 2;
        if(root < rem){
            root++;
            rem -= root;
            root++;
        }
    }
    return root >> 1;
}

// https://serverfault.com/a/333203
long long calc_mdev(int n, long long rtt_sum, long long rtt_sum2) {
	rtt_sum = rtt_sum / n;
	rtt_sum2 = rtt_sum2 / n;
	return ft_sqrt(rtt_sum2 - rtt_sum * rtt_sum);
}
