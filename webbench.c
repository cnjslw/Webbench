    
	
	/*
	webbench��
		�򵥵���վѹ�����Թ���
		ͨ�������ӽ���ģ��ͻ��ˣ���һ��ʱ������Ŀ����վ�ظ�����get���󣬲����㷵�ص�����
	*/
	
	
    /*
     * (C) Radim Kolar 1997-2004
     * This is free software, see GNU Public License version 2 for
     * details.
     *
     * Simple forking WWW Server benchmark:
     *
     * Usage:
     *   webbench --help
     *
     * Return codes:
     *    0 - sucess
     *    1 - benchmark failed (server is not on-line)
     *    2 - bad param
     *    3 - internal error, fork failed
     *
     ���ʹ��webbench ����Դ��������webbench����shell��ʹ������
�������С�webbench -c 100 -t 60 http��//www��baidu��com/��ע��ĩβ��'/'�����١���
     */
	
	/* ���socket��c�ļ����Լ�д��*/
    #include "socket.c"
    #include <unistd.h>
    #include <sys/param.h>
    #include <rpc/types.h>
    #include <getopt.h>
    #include <strings.h>
    #include <time.h>
    #include <signal.h>
    
     /* values */
     //���Ʋ��Ե�ʱ��������benchtimeʱ�䣬ͨ������SIGALARM�źŸ��ӽ��̣��ӽ��̵��ú���alarm_handler����timerexpired��Ϊ1��timerexpired����������ӽ���ֹͣ���Եĺ��̵ơ�
    volatile int timerexpired = 0;
    
    /*
	���ԵĽ������ʵ��һ���ṹ���ʾ�����ơ�
	�ӽ��̰����ǵĸ��Ե�speed��failed��bytesд���ܵ��ļ�������ӽ��̲�������Ժ󣬸����̰ѹܵ����������struct��int speed��int failed��bytes ����������������е�Ԫ�ؽ��л��ܣ����������speed��failed��bytes��
	*/
    int speed = 0;
    int failed = 0;
    int bytes = 0;
    
    int http10 = 1; /* 0 - http/0.9, 1 - http/1.0, 2 - http/1.1 */
    /* 
	method: GET, HEAD, OPTIONS, TRACE������Щ���Ǵ������в��������û�����ʱ��ʹ�ã� ������Щ�����󲿷���������ȡ�û�����ġ���getopt_long��ʹ��
	*/
    #define METHOD_GET 0
    #define METHOD_HEAD 1
    #define METHOD_OPTIONS 2
    #define METHOD_TRACE 3
    #define PROGRAM_VERSION "1.5"
    int method = METHOD_GET;	
    
    int clients = 1;			//��Ҫģ��Ŀͻ��˵ĸ���������Ҫ�������ӽ�����
    int force = 0;				//�����Ƿ���շ�������Ӧ��0��ʾ����
    int force_reload = 0;		
    int proxyport = 80;
    char *proxyhost = NULL;
    int benchtime = 30;			//ģ��ͻ��˵�����ʱ��
    
    int mypipe[2];
	//������ַ���ַ���
    char host[MAXHOSTNAMELEN];
    #define REQUEST_SIZE 2048	
	
	/* 
		����Ƿ��͸���������http������ַ��������ں����ø���ƴ���ַ����ĺ���strcat֮���ƴ�ӳ����ġ�
		ͬʱ����ַ���Ҳ�ǻᱻwrite��sock��request��sizeof��request�����ĺ������͸��������ġ�
	*/
    char request[REQUEST_SIZE];
    
	/*
    struct option{
    	//������ȫ��
    	const char *name;
    	//ѡ���Ƿ���Ҫ����: no_argument(0)/required_argument(1)/optional_argument(2)��Щ���Ǻꡣ
    	int has_arg;
    	//���flagΪNULL��getopt_long���ؽṹ����val��ֵ�����flag��Ϊ��ָ��NULL��getopt_long����0��flagָ����ָ�����ֵval�����û�з��ֳ�ѡ���flag��ָ��ֵ���䡣
    	int *flag;
    	//���ֳ�ѡ���ķ���ֵ��һ��Ϊ��ѡ���ַ�������������flag��ΪNULLʱ������flag��ֵ
    	int val;
    }
    */
    static const struct option long_options[] =
    {
    //�û��������--forceѡ���ѵ���������force���ȫ�ֱ�����ֵ��ֵΪ1�������������Ϳ��Ը����û���ѡ����������
     {"force",no_argument,&force,1},
    
     {"reload",no_argument,&force_reload,1},
     {"time",required_argument,NULL,'t'},
     {"help",no_argument,NULL,'?'},
     {"http09",no_argument,NULL,'9'},
     {"http10",no_argument,NULL,'1'},
     {"http11",no_argument,NULL,'2'},
     {"get",no_argument,&method,METHOD_GET},
     {"head",no_argument,&method,METHOD_HEAD},
     {"options",no_argument,&method,METHOD_OPTIONS},
     {"trace",no_argument,&method,METHOD_TRACE},
     {"version",no_argument,NULL,'V'},
     {"proxy",required_argument,NULL,'p'},
     {"clients",required_argument,NULL,'c'},
     {NULL,0,NULL,0}
    };
    
    /* prototypes */
	//���Ժ��ĺ�������bench�����ڵ���
    static void benchcore(const char* host, const int port, const char *request);
	
	//���Ժ������ڻ�ȡ�û��ĸ��������Ժ���á�
    static int bench(void);
	
	//���û������webbench -c 100 -t 60 http��//www��baidu��com/��������Ĳ�������������
    static void build_request(const char *url);
	
	//����ĺ�����ע���sigalrm�źŵĴ���������
    static void alarm_handler(int signal)
    {
    	timerexpired = 1;
    }
	
    //����Ϊ�û������˴��������ѡ��Ͳ�������ʾ�û��ġ�
    static void usage(void)
    {//��ӡ��st derr��������Ļ����ʾ����
    	fprintf(stderr,
    		"webbench [option]... URL\n"
    		"  -f|--force               Don't wait for reply from server.\n"
    		"  -r|--reload              Send reload request - Pragma: no-cache.\n"
    		"  -t|--time <sec>          Run benchmark for <sec> seconds. Default 30.\n"
    		"  -p|--proxy <server:port> Use proxy server for request.\n"
    		"  -c|--clients <n>         Run <n> HTTP clients at once. Default one.\n"
    		"  -9|--http09              Use HTTP/0.9 style requests.\n"
    		"  -1|--http10              Use HTTP/1.0 protocol.\n"
    		"  -2|--http11              Use HTTP/1.1 protocol.\n"
    		"  --get                    Use GET request method.\n"
    		"  --head                   Use HEAD request method.\n"
    		"  --options                Use OPTIONS request method.\n"
    		"  --trace                  Use TRACE request method.\n"
    		"  -?|-h|--help             This information.\n"
    		"  -V|--version             Display program version.\n"
    	);
    };



	//�����Ǻ���������������������
    int main(int argc, char *argv[])
    {

	
    	int opt = 0;
    	int options_index = 0;
		//���tmp���ַ�����ָ�������������ʱ���ַ������ڽ���request�ַ���ʱ��ʹ�õġ�
    	char *tmp = NULL;
		//����û�г���������Ĳ��������ӡ˵�����˳�
    	if (argc == 1)
    	{
    		usage();
    		return 2;
    	}
    /*���������в������ж����룬������Ӧ���������ã���./webbench -c 100 -t 60 http://www.baidu.com/ Ϊ��*/
    	while ((opt = getopt_long(argc, argv, "912Vfrt:p:c:?h", long_options, &options_index)) != EOF)
    	{
    		switch (opt)
    		{
    		case  0: break;
    		case 'f': force = 1; break;
    		case 'r': force_reload = 1; break;
    		case '9': http10 = 0; break;
    		case '1': http10 = 1; break;
    		case '2': http10 = 2; break;
    		case 'V': printf(PROGRAM_VERSION"\n"); exit(0);
    		case 't': benchtime = atoi(optarg); break;
    		case 'p':
    			/* proxy server parsing server:port */
    			/*
				strrchr(const char * s ,int c):�����ַ����ַ��������һ�γ��ֵ�λ�ã����ظ��ַ����������ַ�����
				����strrchr(cabbbab��a)���ص�λ����ca���a���ڵ�λ�á�
				*/
				
			//�û��� ������IP��ַ��ð�żӶ˿ںţ�����ͨ����ð����ȷ���ַ�����ip��ַ�Ͷ˿ںŷֿ���λ�� 127.0.0.1��80
    			tmp = strrchr(optarg, ':');
			//��proxyhostȥ��ȡ�û������ip��ַ�Ͷ˿ڵ��ַ�����optarg������whileѭ�����仯
				proxyhost = optarg;
			//����"��"ʧ���ˣ�û�ҵ��������û�����������ˣ��˳�
    			if (tmp == NULL)
    			{
    				break;
    			}
			//ð������ǰ�棺80��û��д��IP��ַ�������û����룺127.0.0.1��80
    			if (tmp == optarg)
    			{
    				fprintf(stderr, "Error in option --proxy %s: Missing hostname.\n", optarg);
    				return 2;
    			}
			//ð�ź���Ķ˿����ݲ���
			//���磺8,����Ҫ��λ
    			if (tmp == optarg + strlen(optarg) - 1)
    			{
    				fprintf(stderr, "Error in option --proxy %s Port number is missing.\n", optarg);
    				return 2;
    			}
   /*
   *tmp='/0' 
   ��ð�ű��0�����磺80�����080Ȼ����ת����Ϊ���֡�
   �������Ҳ²���Ϊ��ת��ǰ���IP��ַ����127.0.0.1��80���127.0.0.1 0 80 ��������ַ�����8ǰ�����ָ�������ַ���������ip��ַ��ת����
���������뵽��ϸ�ڣ�����ȥ���õĻ��ͷǳ��˷�ʱ�䡣�ر���û��ע�͵�����¡������Ҿ��ô��͵���Ŀ������˵�ں˻������Լ�ȥд���ҹ�ȥ�������ʱ�䶼���������˼ҵĴ������棬̫���ˡ�����Ҳû��ʲô�ɹ���
   */
    			*tmp = '\0';
	//tmp+1�ǰ�ð�ź����80ת������֣�������proxyport���档
    			proxyport = atoi(tmp + 1); break;
    		case ':':
    		case 'h':
    		case '?': usage(); return 2; break;
    		case 'c': clients = atoi(optarg); break;
    		}
    	}
    
    	if (optind == argc) {
    		fprintf(stderr, "webbench: Missing URL!\n");
    		usage();
    		return 2;
    	}
    	//��ֹ�û���clients��benchtime����Ϊ0
    	if (clients == 0) clients = 1;
    	if (benchtime == 0) benchtime = 60;//Ĭ������ʱ��60
    	//�����Ȩ��Ϣ
    	fprintf(stderr, "Webbench - Simple Web Benchmark "PROGRAM_VERSION"\n"
    		"Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.\n"
    	);
   
	/*
	����getopt_longִ����whileѭ�������argv��optind��ָ���Ǹ��ٶȵ���ַ���ַ�����
	���Ծ�������ַ����������Ϲ���HTTP��request����Ϣ��
	����û�ŵ�ȫ�ֱ���request���ַ���������
	*/
    	build_request(argv[optind]);//����������Ϣ��
    	
    	/* print bench info *///�����û������method��������������ӡʲô��Ϣ������
    	printf("\nBenchmarking: ");
    	switch (method)
    	{
    	case METHOD_GET:
    	default:
    		printf("GET"); break;
    	case METHOD_OPTIONS:
    		printf("OPTIONS"); break;
    	case METHOD_HEAD:
    		printf("HEAD"); break;
    	case METHOD_TRACE:
    		printf("TRACE"); break;
    	}
    //�������ʾ���ٶ���ַ
    	printf(" %s", argv[optind]);
    	switch (http10)
    	{
    	case 0: printf(" (using HTTP/0.9)"); break;
    	case 2: printf(" (using HTTP/1.1)"); break;
    	}
    	printf("\n");
    	if (clients == 1) printf("1 client");
    	else
    		printf("%d clients", clients);
    
    	printf(", running %d sec", benchtime);
    	if (force) printf(", early socket close");
    	if (proxyhost != NULL) printf(", via proxy server %s:%d", proxyhost, proxyport);
    	if (force_reload) printf(", forcing reload");
    	printf(".\n");
    //ִ�в��Թ������󲿷ֵ����ݣ��������ݶ���bench�������
    	return bench();
    }
    
    /*����������Ϣ��*/
    void build_request(const char *url)//�β�ʵ��ʹ�õĺ��������ǣ�optindָ���argv����Ԫ��
    {	//��ʱ���ַ������飬��װrequest�����ʱ���á�
    	char tmp[10];
    	int i;
		/*
		bzero�����������ǽ�sָ��ָ��ĵ�ַ��n���ֽ����㡣
		bzero��ͷ�ļ���#include <string.h>
		*/
    	bzero(host, MAXHOSTNAMELEN);
    	bzero(request, REQUEST_SIZE);
    
    	if (force_reload && proxyhost != NULL && http10 < 1) http10 = 1;
    	if (method == METHOD_HEAD && http10 < 1) http10 = 1;
    	if (method == METHOD_OPTIONS && http10 < 2) http10 = 2;
    	if (method == METHOD_TRACE && http10 < 2) http10 = 2;
		//��get��Щ��Ϣ�ؼ��ַ���request�ַ�������ǰ��
    	switch (method)
    	{
    	default:
    	case METHOD_GET: strcpy(request, "GET"); break;
    	case METHOD_HEAD: strcpy(request, "HEAD"); break;
    	case METHOD_OPTIONS: strcpy(request, "OPTIONS"); break;
    	case METHOD_TRACE: strcpy(request, "TRACE"); break;
    	}
    
    	/*
		ǰ����strc py��������Ϊrequest�����ǿյġ�
		����request�����ж����ˣ�����ֻ��ƴ���ˡ�
		strcat���ַ������Ӻ���������ֱ�����ӡ�
		���´���request�Ӹ��ո�request��������get���get+" "
		*/
    	strcat(request, " ");
    	
    	/*
		������strchr�ǲ����ַ��������򷵻ص�ַ�����򷵻�NULL��
		url��optindָ����ַ���������������Ĳ�����
		��������ʾ�ַ�����û�йؼ��ĵ�ַ����
		*/
    	if (NULL == strstr(url, "://"))
    	{
    		fprintf(stderr, "\n%s: is not a valid URL.\n", url);
    		exit(2);
    	}
		/*
		��������ʾ�ַ���̫���ˣ�Ҳ����argv��optind��̫���ˣ�Ҳ�����û��������ַ̫���ˣ�
		���Գ����������˳���Ԥ�軺�����Ĵ�С��
		��ֹ�����������һ�ְ�ȫ��ʩ����linux c���ʵսP309
		*/
    	if (strlen(url) > 1500)
    	{
    		fprintf(stderr, "URL is too long.\n");
    		exit(2);
    	}
    	/*
    	���proxyhost������host���ǿ���ΪNULL�ġ�
		��Ϊֻ��ʹ����-pѡ��󣬲ſ�����֮ǰ��proxy��ֵ����Ȼ����һֱΪNULL
    	�������ʹ�ô�������ô�Ͳ���ʹ��http֮���Э��
    	*/
    /*
		strncasecmp����
		ͷ�ļ���#include <string.h>
    �����������壺int strncasecmp(const char *s1, const char *s2, size_t n)
    ��������˵����strncasecmp()�����Ƚϲ���s1��s2�ַ���ǰn���ַ����Ƚ�ʱ���Զ����Դ�Сд�Ĳ���
    ��������ֵ ��������s1��s2�ַ�����ͬ�򷵻�0 s1������s2�򷵻ش���0��ֵ s1��С��s2�򷵻�С��0��ֵ 
	*/
    	if (proxyhost == NULL)

    		if (0 != strncasecmp("http://", url, 7))
    		{
    			fprintf(stderr, "\nOnly HTTP protocol is directly supported, set --proxy for others.\n");
    			exit(2);
    		}
    	/* protocol/host delimiter */
    	
		/*
		strstr��str1��str2������str2��str1�е�λ��,str2��str1��λ��
		��http://www.baidu.com
		-url�õ�Э�鳤�� ��http�ĳ���
		+3���Ϊ����֮ǰ�ĳ���  ��http:// 
		*/
    	i = strstr(url, "://") - url + 3;
    	/*�����ڴ������������仰��i��ӡ������ printf("%d\n��i��*/
    
    	
        /*�����i����Ҫ��Ϊ�˷ָ��ַ�����
        1.
        ��ô�����url+i���Ǵ�://���濪ʼ���ַ���
        ������strchr�ǲ����ַ��������򷵻ص�ַ�����򷵻�NULL��
        ������δ����ǲ���http��//www��baiu��com/�Ƿ��д������ַ�����
        URL�﷨����--hostnameû���� / ��β
        2.�ҿ������������ر����ѣ�һ������������ / �������Ϊ������������������û����� / �ǻ��˳������
        3.�����ǲ��Ǵ��������ﶼ����һ����URLҪ / ������
    	*/
		
		//������url+i����˼������http/�����/
    	if (strchr(url + i, '/') == NULL) {
    		fprintf(stderr, "\nInvalid URL syntax - hostname don't ends with '/'.\n");
    		exit(2);
    	}
    
    	//û��ʹ�ô���������
    	if (proxyhost == NULL)
    	{
    		/* get port from hostname */
    		/* �ж˿����� http://www.baidu.com:3300/ */
    		//index�ǲ����ַ����е�һ�����ֵ��ַ��������ظ��ַ��ĵ�ַ�������Ϻ�strchr()������һ����
			//ע������url+l���ƶ��ַ���ָ�룬���ڶ���ַ����һ��������ȡ�Ӽ���
    		if (index(url + i, ':') != NULL						//����ж����û�������˿� 
			&& index(url + i, ':') < index(url + i, '/'			//������ж����û�����Ķ˿ڵ�ð���ڽ�����|��֮ǰ)
			)) 
    		{
    			
			//host�Ͷ˿��Ƿֿ������ģ�������仰�ǿ���host�ġ��������� http://www.baidu.com:3300/�е� www.baidu.com
    			strncpy(host, url + i, strchr(url + i, ':') - url - i);
    			bzero(tmp, 10);
/*
 ����仰�ֽ�
 strncpy(tmp, index(url + i, ':') + 1�����ַ�ǣ�3300�����ð�ŵĵ�ַ+1 Ҳ����3��λ�ã������λ���������˿� ,
 �����Ǽ��㿽������ strchr(url + i, '/')�����3300/������/�ĵ�ַ - index(url + i, ':')����ǣ�3300��ð�ŵĵ�ַ - 1)����������Ҫ�����ĵ�ַ�����ˡ�;
 */
 
    			strncpy(tmp, index(url + i, ':') + 1, strchr(url + i, '/') - index(url + i, ':') - 1);	//���ƶ˿ں�
				
    			//�����������������tmp��ֵ�ģ������õ� printf("tmp=%s\n",tmp); 
    			//��Ϊ���URL: http://www.baidu.com:/ �������tmp����NULL��
    			proxyport = atoi(tmp);
				//���û�û������˿ںŵ�ʱ��tmpΪ0������proxyportҲΪ0�����Ծ�Ĭ��ʹ��80�Ķ˿ڡ�
    			if (proxyport == 0) proxyport = 80;
    		}
    		else
    		{
    			/*
				����Ĵ����ǻ�ȡ���û�ֻ�����ַû�ж˿ڵ����htt��//www��baidu��com��
				����ֱ�Ӱ�www��baidu��com�������ַ���������host�ַ�����
				host�ַ����Ƿŵ�socket�Ǹ�����ȥʹ�õģ�������requset�ַ�������Ҫ������
    			������ / ֮�����������ip��ַ���ں�����ʼ���ͽ�host����ˣ���������ĸ��Ʋ��õ��ģ����磺 http://www.baidu.com/
    			strcspn(a,b)�������᷵��a�дӿ�ͷ��������b�ַ����ĸ���ע�������url+i��ָ�� http://��ĩβ�ġ�	
				*/
    			strncpy(host, url + i, strcspn(url + i, "/"));
    		}
    		//������������� printf("Host=%s\n",host)����host��ֵ;
			//ע��request + strlen(request)��ʾǰ���Ѿ�д�õ��ַ�����������Ҫ���۸�
    		strcat(request + strlen(request), url + i + strcspn(url + i, "/"));
    	}
    	//ʹ�ô���������
    	else
    	{
    		// printf("ProxyHost=%s\nProxyPort=%d\n",proxyhost,proxyport);
    		/*
    		strcat(a,b)�����ǻὫb����a�ĺ��棬���Ḳ��ԭ���ַ���a������
    		*/
    		strcat(request, url);	//ʹ���˴������������������б�����������UIR
    	}
    	if (http10 == 1)
    		strcat(request, " HTTP/1.0");
    	else if (http10 == 2)
    		strcat(request, " HTTP/1.1");
		]
    	strcat(request, "\r\n");
    	if (http10 > 0)
    		strcat(request, "User-Agent: WebBench "PROGRAM_VERSION"\r\n");
    	if (proxyhost == NULL && http10 > 0)
    	{
    		strcat(request, "Host: ");
    		strcat(request, host);
    		strcat(request, "\r\n");
    	}
    	if (force_reload && proxyhost != NULL)
    	{
    		strcat(request, "Pragma: no-cache\r\n");
    	}
    	if (http10 > 1)
    		strcat(request, "Connection: close\r\n");
    	/* add empty line at end */
    	if (http10 > 0) strcat(request, "\r\n");
    	// printf("Req=%s\n",request);
    }
    
    /* vraci system rc error kod */
    /*
    �����̴���clients���ӽ��̣����ӽ���ͨ��pipeͨ��
    */
    static int bench(void)
    {
    	int i, j, k;
    	pid_t pid = 0;
    //�����f����ȥ�򿪶�Ӧ�ܵ��ļ��ġ�
    	FILE *f;
    
    	/* check avaibility of target server */
    	//�����proxyport��������proxyhost�Ķ˿ڣ�Ҳ����ʵip�Ķ˿�
    	i = Socket(proxyhost == NULL ? host : proxyhost, proxyport);
    	if (i < 0) {
    		fprintf(stderr, "\nConnect to server failed. Aborting benchmark.\n");
    		return 1;
    	}
    	//Ϊʲô����һ����Ҫ���ļ�������i���رգ�
    	//����Ҳ�ǿ��Բ�����رյģ���Ϊû�д���д�����ݸ�����������������͹ر���
    	close(i);
    	/* create pipe */
    	if (pipe(mypipe))
    	{
    		perror("pipe failed.");
    		return 3;
    	}
    
    	/* not needed, since we have 
    	() in childrens */
    	/* wait 4 next system clock tick */
    	/*time������ȡ��ǰ��ʱ�䣬�������ע�͵�������ڵ�ǰ��ʱ������׽��̲�ִ�еģ��ó�ִ��ʱ��ġ�sched--yield���������˼��
    	cas=time(NULL);
    	while(time(NULL)==cas)
    		  sched_yield();
    	*/
    
    	/* fork childs */
    	for (i = 0; i < clients; i++)
    	{
    		pid = fork();
    		if (pid <= (pid_t)0)
    		{
    			/* child process or error*/
    			sleep(1); /* make childs faster */
    			break;
    		}
    	}
    
    	if (pid < (pid_t)0)
    	{
    		fprintf(stderr, "problems forking worker no. %d\n", i);
    		perror("fork failed.");
    		return 3;
    	}
    
    	if (pid == (pid_t)0)
    	{
    		/* I am a child ����proxyhost��ֵ���ж��Ƿ�ʹ���˴���*/
    		if (proxyhost == NULL)
    			benchcore(host, proxyport, request);
    		else
    			benchcore(proxyhost, proxyport, request);
    
    		/* write results to pipeע���������ӽ��̴�pipe��д�ˣ�дspeed��failed��byte����ֵ��ȥ */
    		f = fdopen(mypipe[1], "w");
    		if (f == NULL)
    		{
    			perror("open pipe for writing failed.");
    			return 3;
    		}
    		/* fprintf(stderr,"Child - %d %d\n",speed,failed); */
    /*
	speed��bytes��fail��benchcore���������Ӻͽ������������ĵġ�
	�����ǲ������֮��ÿ���ӽ������ܵ���д���ݣ����㸸���̶������������ܡ�
	�ܵ���ͨ��������Ԫ�صĽṹ���������������ݵġ�
    */
    		fprintf(f, "%d %d %d\n", speed, failed, bytes);
    		fclose(f);
    		return 0;
    	}//�ӽ��̵Ĵ��뵽���������
    	else
    	{	//ע���ӽ��̲��Խ���֮��Ż����ܵ�д���ݣ����Ը��׽���ֻ��Ҫһֱȥ���ܵ����ȡ���ݾͿ����ˡ�
    	    /*I am a parent*/
    		f = fdopen(mypipe[0], "r");
    		if (f == NULL)
    		{
    			perror("open pipe for reading failed.");
    			return 3;
    		}
    //�����ö��Ļ���
    //��ʹ�ÿ⺯�����κλ�����ơ����ǲ�֪��ΪʲôҪ����仰
    		setvbuf(f, NULL, _IONBF, 0);
    //��ʼ�����Ҫ����ĸ��׽��̵�speed��failed��bytes����ʵ������������f-speed���á������������������ӽ��̵�speed�����������⡣
			speed = 0;
    		failed = 0;
    		bytes = 0;
    
    		while (1)
    		{//�ӹܵ�������ݣ�һ�ζ�����������֮�����ݾͻ���ʧ��
    			pid = fscanf(f, "%d %d %d", &i, &j, &k);
    			if (pid < 2)
    			{
    				fprintf(stderr, "Some of our childrens died.\n");
    				break;
    			}
    //ijk����������ʱ�����洢speed��fail byte��ֵ�ġ�
    			speed += i;
    			failed += j;
    			bytes += k;
    			/* fprintf(stderr,"*Knock* %d %d read=%d\n",speed,failed,pid); */
    //�ж��ٸ��ӽ��̾Ͷ�ȡ���ٴ�
    			if (--clients == 0) break;
    		}
    		fclose(f);
    //���ѽ��ת������ʾ����
    		printf("\nSpeed=%d pages/min, %d bytes/sec.\nRequests: %d susceed, %d failed.\n",
    			(int)((speed + failed) / (benchtime / 60.0f)),
    			(int)(bytes / (float)benchtime),
    			speed,
    			failed);
    	}
    	return i;
    }
    /*
    	�ӽ���ͨ��socket�򱻲�������������󣬸ù��̻�ı�speed��failed��bytes��ֵ����ִ����benchcore�Ժ���Щֵ��ͨ���ܵ����ܡ�
    */
    void benchcore(const char *host, const int port, const char *req)
    {
    	int rlen;
    	char buf[1500];
   // s��sock ��i���������վ��ȡ�������ݵ�
    	int s, i;
    	struct sigaction sa;
    
    	/* setup alarm signal handler */
    	sa.sa_handler = alarm_handler;
    	sa.sa_flags = 0;
    	if (sigaction(SIGALRM, &sa, NULL))
    		exit(3);
    	alarm(benchtime);
    
    	rlen = strlen(req);
    nexttry:while (1)
    {
    	//ʱ�䵽�󣬻���ת��alarm_handler()��������timerexpired����Ϊ 1
    	if (timerexpired)
    	{
    		//Ϊʲô��ʱ��Ҫfailed--?��Ϊ��ε�����ʧ����ʱ�䵽����ɵġ�
    		if (failed > 0)
    		{
    			/* fprintf(stderr,"Correcting failed by signal\n"); */
    			failed--;
    		}
    		return;
    	}
    //Socket����������Ķ���
    	s = Socket(host, port);
    	if (s < 0) { failed++; continue; }	//����ʧ�ܣ�failed++
    //����write�ĺ���s���������sock����������sock��д����
    	if (rlen != write(s, req, rlen)) { failed++; close(s); continue; }	//�������д������ʧ�ܣ�failed++
    	if (http10 == 0)
    		if (shutdown(s, 1)) { failed++; close(s); continue; }	//��ֹд����������ݲ������ɹ�Ϊ0��ʧ��Ϊ-1
    
    	/*force==0������ȡ������������������*/
    	if (force == 0)
    	{
    		/* read all available data from socket */
    		while (1)
    		{
    			if (timerexpired) break;
    			i = read(s, buf, 1500);
    			/* fprintf(stderr,"%d\n",i); */
    			/*��ȡ���������͹���������ʧ�ܣ�failed++*/
    			if (i < 0)
    			{
    				failed++;
    				close(s);
					//�������Ӳ�����
    				goto nexttry;
    			}
    			else
    				if (i == 0) break;
    				else
    					bytes += i;
    		}
    	}
		//sock�޷������رգ�ʧ�ܼ�1
    	if (close(s)) { failed++; continue; }
    	speed++;
    }
    }


