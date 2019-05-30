#ifndef BUTTON_H
#define BUTTON_H

//��Բ�ͬ�ı��������ò�ͬ��stdint.h�ļ�
#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
#include <stdint.h>
#endif


#define BTN_NAME_MAX 32 //�������Ϊ32�ֽ�

/* ��������ʱ��40ms, �����������Ϊ20ms
 ֻ��������⵽40ms״̬�������Ϊ��Ч����������Ͱ��������¼�
*/

#define CONTINUOS_TRIGGER 0 //�Ƿ�֧�����������������Ļ��Ͳ�Ҫ��ⵥ˫���볤����

/* �Ƿ�֧�ֵ���&˫��ͬʱ���ڴ��������ѡ�����궨��Ļ�����˫�����ص���ֻ�����������ӳ���Ӧ��
   ��Ϊ�����жϵ���֮���Ƿ񴥷���˫�������ӳ�ʱ����˫�����ʱ�� BUTTON_DOUBLE_TIME��
   ���������������궨�壬���鹤����ֻ���ڵ���/˫���е�һ����������˫����Ӧ��ʱ��ᴥ��һ�ε�����
   ��Ϊ˫����������һ�ΰ��²����ͷ�֮��Ų����� 
*/
#define SINGLE_AND_DOUBLE_TRIGGER 1

/* �Ƿ�֧�ֳ����ͷŲŴ��������������궨�壬��ô�����ͷ�֮��Ŵ������γ�����
   �����ڳ���ָ��ʱ���һֱ�������������������� BUTTON_LONG_CYCLE ���� 
*/
#define LONG_FREE_TRIGGER 0

#define BUTTON_DEBOUNCE_TIME      2       /* ����ʱ��            (n-1)*�������� */
#define BUTTON_CONTINUOS_CYCLE    1       /* ������������ʱ��     (n-1)*�������� */
#define BUTTON_LONG_CYCLE         1       /* ������������ʱ��     (n-1)*�������� */
#define BUTTON_DOUBLE_TIME        15      /* ˫�����ʱ��         (n-1)*��������  ������200-600ms */
#define BUTTON_LONG_TIME          50      /* ����n��             ((n-1)*�������� ms)����Ϊ�����¼� */

#define TRIGGER_CB(event)            \
  if (btn->CallBack_Function[event]) \
  btn->CallBack_Function[event]((Button_t *)btn)

typedef void (*Button_CallBack)(void *); /* ���������ص���������Ҫ�û�ʵ�� */

typedef enum
{
  BUTTON_DOWM = 0,
  BUTTON_UP,
  BUTTON_DOUBLE,
  BUTTON_LONG,
  BUTTON_LONG_FREE,
  BUTTON_CONTINUOS,
  BUTTON_CONTINUOS_FREE,
  BUTTON_ALL_RIGGER,
  number_of_event, /* �����ص����¼� */
  NONE_TRIGGER
} Button_Event;

/*
	ÿ��������Ӧ1��ȫ�ֵĽṹ�������
	���Ա������ʵ���˲��Ͷ��ְ���״̬�������
*/
typedef struct button
{
  /* ������һ������ָ�룬ָ���жϰ����ַ��µĺ��� */
  unsigned char (*Read_Button_Level)(void); /* ��ȡ������ƽ��������Ҫ�û�ʵ�� */

  char Name[BTN_NAME_MAX];

  unsigned char Button_State          : 4;          /* ������ǰ״̬�����»��ǵ��� */
  unsigned char Button_Last_State     : 4;          /* ��һ�εİ���״̬�������ж�˫�� */
  unsigned char Button_Trigger_Level  : 2;          /* ����������ƽ */
  unsigned char Button_Last_Level     : 2;          /* ������ǰ��ƽ */

  unsigned char Button_Trigger_Event;               /* ���������¼���������˫���������� */

  Button_CallBack CallBack_Function[number_of_event];

  unsigned char Button_Cycle;           /* ������������ */

  unsigned char Timer_Count;            /* ��ʱ */
  unsigned char Debounce_Time;          /* ����ʱ�� */

  unsigned char Long_Time;              /* �������³���ʱ�� */

  struct button *Next;

} Button_t;

#define PRINT_DEBUG_ENABLE    0    /* ��ӡ������Ϣ */
#define PRINT_ERR_ENABLE      0    /* ��ӡ������Ϣ */
#define PRINT_INFO_ENABLE     1    /* ��ӡ������Ϣ */

#if PRINT_DEBUG_ENABLE
#define PRINT_DEBUG(fmt, args...)                   \
  do                                                \
  {                                                 \
    (printf("\n[DEBUG] >> "), printf(fmt, ##args)); \
  } while (0)
#else
#define PRINT_DEBUG(fmt, args...)
#endif

#if PRINT_ERR_ENABLE
#define PRINT_ERR(fmt, args...)                   \
  do                                              \
  {                                               \
    (printf("\n[ERR] >> "), printf(fmt, ##args)); \
  } while (0)
#else
#define PRINT_ERR(fmt, args...)
#endif

#if PRINT_INFO_ENABLE
#define PRINT_INFO(fmt, args...)                   \
  do                                               \
  {                                                \
    (printf("\n[INFO] >> "), printf(fmt, ##args)); \
  } while (0)
#else
#define PRINT_INFO(fmt, args...)
#endif

/**@} */

/* ���� Assert */
#define AssertCalled(char, int) printf("\nError:%s,%d\r\n", char, int)
#define ASSERT(x) \
  if ((x) == 0)   \
  AssertCalled(__FILE__, __LINE__)

typedef enum
{
  ASSERT_ERR = 0,              /* ���� */
  ASSERT_SUCCESS = !ASSERT_ERR /* ��ȷ */
} Assert_ErrorStatus;

/* ���ⲿ���õĺ������� */

void Button_Create(const char *name,
                   Button_t *btn,
                   unsigned char (*read_btn_level)(void),
                   unsigned char btn_trigger_level);

void Button_Attach(Button_t *btn, Button_Event btn_event, Button_CallBack btn_callback);

void Button_Cycle_Process(Button_t *btn);

void Button_Process(void);

void Button_Delete(Button_t *btn);

void Search_Button(void);

void Get_Button_EventInfo(Button_t *btn);
unsigned char Get_Button_Event(Button_t *btn);
unsigned char Get_Button_State(Button_t *btn);
#endif
