#define NONE "\033[m"

#define RED "\033[0;32;31m"

#define LIGHT_RED "\033[1;31m"

#define GREEN "\033[0;32;32m"

#define LIGHT_GREEN "\033[1;32m"

#define BLUE "\033[0;32;34m"

#define LIGHT_BLUE "\033[1;34m"

#define DARY_GRAY "\033[1;30m"

#define CYAN "\033[0;36m"

#define LIGHT_CYAN "\033[1;36m"

#define PURPLE "\033[0;35m"

#define LIGHT_PURPLE "\033[1;35m"

#define BROWN "\033[0;33m"

#define YELLOW "\033[1;33m"

#define LIGHT_GRAY "\033[0;37m"

#define WHITE "\033[1;37m"

/*************example****************

printf("\033[31m ####-----&gt;&gt; \033[32m" "hello\n" "\033[m")

int main()

{

printf( CYAN "current function is %s " GREEN " file line is %d\n" NONE,

__FUNCTION__, __LINE__ );

fprintf(stderr, RED "current function is %s " BLUE " file line is %d\n" NONE,

__FUNCTION__, __LINE__ );

return 0;

}

��ɫ��Ϊ����ɫ������ɫ��30~39������������ɫ��40~49���ñ�����

����ɫ ����ɫ

40: �� 30: ��

41: �� 31: ��

42: �� 32: ��

43: �� 33: ��

44: �� 34: ��

45: �� 35: ��

46: ���� 36: ����

47: ��ɫ 37: ��ɫ

�ǵ��ڴ�ӡ��֮�󣬰���ɫ�ָ���NONE����Ȼ�ٺ���Ĵ�ӡ������ű�ɫ��

���⣬�����Լ�һЩANSI�����롣����ɫֻ�����¿������е�һ�֣�

\033[0m �ر���������

\033[1m ���ø�����

\033[4m �»���

\033[5m ��˸

\033[7m ����

\033[8m ����

\033[30m -- \033[37m ����ǰ��ɫ

\033[40m -- \033[47m ���ñ���ɫ

\033[nA �������n��

\033[nB �������n��

\033[nC �������n��

\033[nD �������n��

\033[y;xH���ù��λ��

\033[2J ����

\033[K ����ӹ�굽��β������

\033[s ������λ��

\033[u �ָ����λ��

\033[?25l ���ع��

\033[?25h ��ʾ���
*/

