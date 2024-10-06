// StaticLibUtilites.cpp : Определяет функции для статической библиотеки.
//

#include "pch.h"
#include "framework.h"

//


#define __WIN32__
#define __DEBUG__


#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <chrono>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <errno.h>
#include <map>
#include <atomic>

#ifdef __WIN32__
#include <Windows.h>
#include <WinSock2.h> // заголовочный файл, содержащий актуальные реализации функций для работы с сокетами
#include <WS2tcpip.h> // заголовочный файл, который содержит различные программные интерфейсы, связанные с работой протокола TCP/IP (переводы различных данных в формат, понимаемый протоколом и т.д.)
#else
#include <arpa/inet.h> 
#include <sys/socket.h>
#endif

#ifdef __WIN32__
#pragma comment(lib, "Ws2_32.lib") // прилинковать к приложению динамическую библиотеку ядра ОС: ws2_32.dll. Делаем это через директиву компилятору
#endif

#ifdef __DEBUG__
#define DEBUG_TRACE(logger, string) logger.doDebugTrace(string); 
#else 
#define DEBUG_TRACE(logger, string) 
#endif



/// <summary>
/// Класс для логгирования событий через файл и/или консоль 
/// </summary>
class log_t
{
public:
    /// <summary>
    /// Конструктор по умолчанию
    /// Если не задаем имя файла логгирования, выводим тоьько в консоль
    /// </summary>
    log_t() : consoleActive(true)
    {}
    /// <summary>
    /// Конструктор с 3-я параметрами
    /// </summary>
    /// <param name="nameLogFile"> - имя файла логгирования </param>
    /// <param name="consoleActive"> - флаг на вывод в консоль </param>
    log_t(std::string nameLogFile, bool consoleActive) : consoleActive(consoleActive), lastErr(0)
    {
        time_zone = 3; // TO_DO
        logFile.open(nameLogFile.c_str(), std::ios::app); // открываем файл логиррования для дозаписи 
        if (!logFile)
        {
            if(consoleActive) std::cout << "logFile.open fail";
            else std::cerr << "logFile.open fail";//TODO check
        }
    }
    /// <summary>
    /// Метод для записи в лог
    /// </summary>
    /// <param name="log"> - строка лога </param>
    /// <param name="errCode"> - код ошибки (опционально) </param>
    void doLog(std::string log, int errCode = 0x80000000)
    {
        // готовим итоговое сообщение лога
        std::string msg = getTime();
        msg.append(" :: ");
        msg.append(log);
        // если есть код ошибки, добавляем его
        if (errCode != 0x80000000)
        {
            lastErr = errCode; // запоминаем значение ошибки 
            msg.append(" errno: ");
            msg.append(std::to_string(errCode));
        }
        // вывод в консоль
        if (consoleActive) std::cout << msg << '\n';
        // вывод в файл
        if (logFile.is_open()) logFile << msg << '\n';
    }
#ifdef __DEBUG__
    /// <summary>
    /// Метод для записи в лог трейса в режиме отладки
    /// </summary>
    /// <param name="log"> - строка лога </param>
    void doDebugTrace(std::string trace)
    {
        // готовим итоговое сообщение трейса
        std::string msg = getTime();
        msg.append(" :: ");
        msg.append(trace);
        // вывод в консоль
        if (consoleActive) std::cout << trace << '\n';
        // вывод в файл
        if (logFile.is_open()) logFile << trace << '\n';
    }
#endif
    /// <summary>
    /// Метод возврата кода последней ошибки, переданой через метод doLog()
    /// </summary>
    /// <returns> код последней ошибки, переданой через метод doLog() </returns>
    int GetLastErr() const
    {
        return lastErr;
    }
    //деструктор
    ~log_t()
    {
        if (logFile.is_open()) // если файл открыт - закрываем
            logFile.close();
    }
protected : 
    std::ofstream logFile; // файл для логгирования
    bool consoleActive; // флаг вывода в консоль 
	int time_zone; // часовой пояс
    int lastErr; // код последней ошибки

    /// <summary>
    /// метод вывода времени
    /// </summary>
    /// <returns> строка формата "гггг.мм.дд-день недели-чч:мм:сс:мсмс"</returns>
    std::string getTime()
    {
        std::stringstream result; // результат

        bool leap = false; // флаг весокосного года
		
        auto now = std::chrono::system_clock::now().time_since_epoch();// 1970 четверг
        auto msec = std::chrono::duration_cast<std::chrono::milliseconds> (now); // миллисекунды
        auto sec = std::chrono::duration_cast<std::chrono::seconds> (now); // секунды
        auto min = std::chrono::duration_cast<std::chrono::minutes> (now); // минуты
        auto tempHour = std::chrono::duration_cast<std::chrono::hours> (now); // часы
        // больше С++17 нам не дает, дальше решаем сами(не актуально в С++20) 
        int hour = tempHour.count() + time_zone; // +3 московский часовой пояс 

        int totalDay = hour / 24; // общее количество дней
        std::string weekDays; // день недели

        switch ((totalDay + 3) % 7) // начинаем с четверга 1970 года
        {
        case 0:
            weekDays = "monday";
            break;
        case 1:
            weekDays = "tuesday";
            break;
        case 2:
            weekDays = "wednesday";
            break;
        case 3:
            weekDays = "thursday";
            break;
        case 4:
            weekDays = "friday";
            break;
        case 5:
            weekDays = "saturday";
            break;
        case 6:
            weekDays = "sunday";
            break;
        default:
            break;
        }

        int year = 1969; // общее количество лет
        int day = 0; // дни

        while (totalDay > 0) // пока общее количество дней еще есть
        {
            ++year; // плюс год
            leap = year % 4 == 0; // год был весокосный?
            day = totalDay; // предварительно устанавливаем дату
            totalDay -= leap ? 366 : 365; // вычитаем год дней из общего количества 
        }

        int mounth = 0; // месяц устанавливаем в  зависимости от количества дней в этом году
        if (day < 31) mounth = 1;
        else if (leap ? day < 59 : day < 60) { mounth = 2; day -= 31; } 
        else if (leap ? day < 90 : day < 91) { mounth = 3; day -= leap ? 59 : 60; }
        else if (leap ? day < 120 : day < 121) { mounth = 4; day -= leap ? 90 : 91; }
        else if (leap ? day < 151 : day < 152) { mounth = 5; day -= leap ? 120 : 121; }
        else if (leap ? day < 181 : day < 182) { mounth = 6; day -= leap ? 151 : 152; }
        else if (leap ? day < 212 : day < 213) { mounth = 7; day -= leap ? 181 : 182; }
        else if (leap ? day < 243 : day < 244) { mounth = 8; day -= leap ? 212 : 213; }
        else if (leap ? day < 273 : day < 274) { mounth = 9; day -= leap ? 243 : 244; }
        else if (leap ? day < 304 : day < 305) { mounth = 10; day -= leap ? 273 : 274; }
        else if (leap ? day < 334 : day < 335) { mounth = 11; day -= leap ? 304 : 305; }
        else if (leap ? day < 365 : day < 366) { mounth = 12; day -= leap ? 334 : 335; }
        // готовим результат формата "гггг.мм.дд-день недели-чч:мм:сс:мсмс"
        result << std::setfill('0') << year << '.' << std::setw(2) << mounth << '.' << std::setw(2) << day << '-' << weekDays << '-' //дата
            << std::setw(2) << hour % 24 << ':' << std::setw(2) << min.count() % 60 << ':' << std::setw(2) << sec.count() % 60 //время
            << '.' << std::setw(3) << msec.count() % 1000;//милисекунды

        return std::string(result.str());

        // c-вариант
        //time_t t1 = time(NULL);//возвращающая текущее время в формате time_t — количество секунд, прошедших с 00:00 1 января 1970. 
        //tm t = *localtime(&t1);//Функция localtime() позволяет перевести time_t в структуру tm, которая состоит из полей, представляющих отдельно часы, минуты, месяц, год и т. д.
        //printf("%.2d:%.2d:%.2d\n", t.tm_hour, t.tm_min, t.tm_sec);
    }
};


namespace network
{
    /// <summary>
    /// Класс запускающий сетевой интерфейс операционной системы (WINDOWS)
    /// Использует принцип RAII
    /// </summary>
    class RAII_OSsock
{
private :
#ifdef __WIN32__
    static WSADATA wsdata; // Структура WSADATA содержит сведения о реализации сокетов Windows.
    static int countWSAusers;// количество пользователей интерфейса
#endif        
protected :
    struct option_t // опции для сокета
    {
        static constexpr int NON_BLOCK = 1; // неблокирующий сокет
    };
    struct error_t // ошибки сокета
    {
#ifdef __WIN32__
        static constexpr int NON_BLOCK_SOCKET_NOT_READY = WSAEWOULDBLOCK; // сокет не блокирующий, не готов к отправке либо приему
#else
        static constexpr int NON_BLOCK_SOCKET_NOT_READY = EAGAIN; // или EWOULDBLOCK // сокет не блокирующий, не готов к отправке либо приему
#endif    
    };
    /// <summary>
    /// конструктор по умолчанию
    /// </summary>
    RAII_OSsock(log_t& logger)
    {
#ifdef __WIN32__
        if (countWSAusers++ == 0) // если интерфейс еще не открывали и количество пользователей = 0
            if (WSAStartup(MAKEWORD(2, 2), &wsdata))// открыть интерфейс
                logger.doLog("RAII_OSsock - WSAStartup ", GetError());// логгируем ошибку
#endif
    }
    ~RAII_OSsock()
    {
#ifdef __WIN32__
        if (--countWSAusers == 0) // как только работу закончил последний юзер
            WSACleanup(); // закрыть интерфейс
#endif
    }
    /// <summary>
    /// Метод вывода номера последней ошибки
    /// </summary>
    /// <returns> номер последней ошибки </returns>
    int GetError()
    {
#ifdef __WIN32__
        return WSAGetLastError();
#else
        return errno;
#endif
    }
    /// <summary>
    /// Метод установки опций для сокета
    /// </summary>
    /// <param name="socket"> - дескриптор сокета </param>
    /// <param name="option"> - опция</param>
    /// <param name="logger"> - объект для логгированя </param>
    /// <returns> 1 - успех </returns>
    bool setSocketOpt(SOCKET socket, int option, log_t& logger)
    {
        bool result = false;

        switch (option)
        {
        case option_t::NON_BLOCK : // опция по устаеновке неблокирующего сокета
        {
#ifdef __WIN32__
            u_long mode = 0;
            if (ioctlsocket(socket, FIONBIO, &mode))
                logger.doLog("RAII_OSsock - ioctlsocket ", GetError());// логгируем ошибку
            else
                result = true;
#else
            if (ioctl(socket, O_NONBLOCK))
                logger.doLog("RAII_OSsock - ioctl ", GetError());// логгируем ошибку
            else
                result = true;
#endif
        }
        default:
            break;
        }

        return result;
    }
};
    
    #ifdef __WIN32__
    WSADATA RAII_OSsock::wsdata; 
    int RAII_OSsock::countWSAusers = 0;
    #endif
    
    /// <summary>
    /// класс реализует хранение информации о сокете 
    /// </summary>
    class sockInfo_t : public RAII_OSsock
{
    friend class UDP_socket_t; // для метода RecvFrom
    friend class TCP_socketServer_t; // для метода AddClient
protected :
    /// <summary>
    /// Метод перезаписи структуры описывающей сокет. После перезаписи необходимо обновить информцию о сокете вызвав UpdateSockInfo()
    /// </summary>
    /// <returns> указатель на структуру описывающую сокет </returns>
    sockaddr* setSockAddr()
    {
        return &Addr;
    }
    /// <summary>
    /// Метод обновления иноформации о сокете, вызывается после обновления Addr метоом setSockAddr()
    /// </summary>
    void UpdateSockInfo()
    {
        // чистим старую информацию о привязке сокета
        IP_port.first.clear();
        IP_port.second = 0;

        sockaddr_in AddrIN; // Структура описывает сокет для работы с протоколами IP
        memset(&AddrIN, 0, SizeAddr()); 
        memmove(&AddrIN, &Addr, SizeAddr()); //Addr ==>> AddrIN
        
        char bufIP[32];// буфер для строки IP
        memset(bufIP, '\0', sizeof(bufIP));

        // Функция InetNtop преобразует интернет-адрес IPv4 или IPv6 в строку в стандартном формате Интернета
        if (!inet_ntop(AF_INET, &AddrIN, bufIP, sizeof(bufIP)))
        {
            IP_port.first.assign(bufIP);
            IP_port.second = ntohs(AddrIN.sin_port);
        }
        else // логгируем ошибку
            logger.doLog("inet_ntop fail", GetError());
    }
    /// <summary>
    /// Метод обновления иноформации о сокете, вызывается после обновления Addr
    /// </summary>
    /// <param name="ip"> IP адресс в формате "хххх.хххх.хххх.хххх" </param>
    /// <param name="port"> номер порта </param>
    void UpdateSockInfo(std::string ip, unsigned short port)
    {
        IP_port.first.swap(ip);
        IP_port.second = port;
    }
public :
    /// <summary>
    /// Конструктор с одним параметром
    /// </summary>
    /// <param name="logger"> - объект для логгирования ошибок </param>
    sockInfo_t(log_t& logger) : RAII_OSsock(logger), logger(logger)
    {
        memset(&Addr, 0, sizeof(Addr));
        IP_port.first.clear();
        IP_port.second = 0;
        sizeAddr = sizeof(Addr);
    }
    /// <summary>
    /// Конструктор с 2-я параметрами
    /// </summary>
    /// <param name="ip"> - IP адресс в формате "хххх.хххх.хххх.хххх" </param> 
    /// <param name="port"> - номер порта </param>
    /// <param name="logger"> - объект для логгирования ошибок </param> 
    sockInfo_t(std::string ip, unsigned short port, log_t& logger) : sockInfo_t(logger)
    {
        setSockInfo(ip, port);
    }
    ~sockInfo_t()
    {
        memset(&Addr, 0, sizeof(Addr));
        IP_port.first.clear();
        IP_port.second = 0;
    }
    /// <summary>
    /// Метод установки информации о сокете
    /// </summary>
    /// <param name="ip"> - IP адресс в формате "хххх.хххх.хххх.хххх" </param>
    /// <param name="port"> - номер порта</param>
    /// <returns> true - успех; false - неудача </returns>
    bool setSockInfo(std::string ip, unsigned short port)
    {
        bool result = false; // результат
   
        sockaddr_in AddrIN; // Структура описывает сокет для работы с протоколами IP
        AddrIN.sin_family = AF_INET; // Семейство адресов
        AddrIN.sin_port = htons(port); // Номер порта транспортного протокола
        // Функция htons возвращает значение в порядке байтов сети TCP/IP
        
        //AddrIN.sin_addr - Структура IN_ADDR , содержащая транспортный адрес IPv4.
        // константы:
        // NADDR_ANY все адреса локального хоста(0.0.0.0);
        // INADDR_LOOPBAC адрес loopback интерфейса(127.0.0.1);
        // INADDR_BROADCAST широковещательный адрес(255.255.255.255)
        int inet_pton_state =  inet_pton(AF_INET, ip.c_str(), &AddrIN.sin_addr);
        // Функция InetPton преобразует сетевой адрес IPv4 или IPv6 в стандартной форме 
        //представления текста в числовую двоичную форму
        // возврат 1 - удача, 0 - неверная строка, -1 - ошибка

        // проверяем результат работы функции
        if (inet_pton_state == 1) // все отлично
        { 
            memset(&Addr, 0, SizeAddr());
            memmove(&Addr, &AddrIN, SizeAddr()); //AddrIN ==>> Addr
            result = true;
            UpdateSockInfo(ip, port);
        }
        else if (inet_pton_state == 0) // неверно задан IP
            logger.doLog("setSockAddr Fail, invalid IP: " + ip);
        else //inet_pton fail
            logger.doLog("inet_pton Fail,IP: " + ip, GetError());
        
        if (result)// вывод отладочной информации что присвоение прошло успешно
            DEBUG_TRACE(logger, "setSockAddr: " + ip + ':' + std::to_string(port) + "-> OK")

        return result;
    }
    /// <summary>
    /// Метод установки информации о сокете
    /// </summary>
    /// <param name="sockInfo"> - структура описывающая сокет для работы с протоколами IP </param>
    /// <returns></returns>
    void setSockInfo(const sockInfo_t& sockInfo)
    {
        memset(&Addr, 0, SizeAddr());
        memmove(&Addr, &sockInfo.Addr, SizeAddr());
        IP_port = sockInfo.IP_port;
    }
    /// <summary>
    /// Метод возвратата константного указателя на структуру описывающую сокет 
    /// </summary>
    /// <returns> константный указатель на структуру описывающую сокет </returns>
    const sockaddr* getSockAddr() const
    {
        return &Addr;
    }
    /// <summary>
    /// Метод возврата размера структуры описывающей сокет 
    /// </summary>
    /// <returns> размер структуры описывающей сокет </returns>
    size_t SizeAddr() const
    {
        return sizeAddr;
    }
    /// <summary>
    /// Метод возврата IP
    /// </summary>
    /// <returns> IP адресс в формате "хххх.хххх.хххх.хххх" </returns>
    std::string GetIP() const
    {
        return IP_port.first;
    }
    /// <summary>
    /// Метод возврата номера порта
    /// </summary>
    /// <returns> номер порта </returns>
    unsigned short GetPort() const
    {
        return IP_port.second;
    }
    /// <summary>
    /// оператор сравнения
    /// </summary>
    /// <param name="rValue"> - правостороннее значение </param>
    /// <returns> 1 - объекты равны </returns>
    bool operator == (const sockInfo_t& rValue) const
    {// сравниваем адреса структур sockaddr (точно уникальные)
        return getSockAddr() == rValue.getSockAddr();
    }
    /// <summary>
    /// оператор сравнения
    /// </summary>
    /// <param name="rValue"> - правостороннее значение </param>
    /// <returns> 1 - объекты не равны </returns>
    bool operator != (const sockInfo_t& rValue) const
    {
        return !(*this == rValue);
    }
protected:
    std::pair<std::string, unsigned short> IP_port; // IP адрес и номер порта
    sockaddr Addr; // стандартная структура для хранения информации о сокете
    size_t sizeAddr; // размер структуры sockaddr
    log_t& logger; // объект для логгирования ошибок
};
    
    /// <summary>
    /// Класс реализует хранение и работу сокета
    /// </summary>
    /// TODO Работа с DNS
    /// getaddrinfo(char const* node, char const* service, struct addrinfo const* hints, struct addrinfo** res)
    class socket_t : public sockInfo_t
{
    friend class NonBlockSocket_manager_t; // менеджер неблокирующмх сокетов, использует setNonBlock
protected :
    /// <summary>
    /// Метод замены дескриптора сокета (для внутреннего пользования серверным сокетом TCP)
    /// </summary>
    /// <param name="socket"> - дескриптор сокета </param>
    /// <returns> true - при удачной замене, false - при неудачной </returns>
    bool SetSocket(SOCKET socket)
    {
        bool result = false;
        // если аргумент валидный
        if (socket != INVALID_SOCKET)
            if (Close()) // закрываем старый сокет
            {
                Socket = socket;
                nonBlock = false; // accept вернет дефолтный блокирующий сокет  
                // обновляем информацию о сокете
                int sizeAddr = SizeAddr();
                if (!getsockname(Socket, setSockAddr(), &sizeAddr))
                    UpdateSockInfo();// обязательно после setSockAddr()
                else
                    logger.doLog("getsockname fail", GetError());

                result = true;
            }

        return result;
    }
    /// <summary>
    /// Метод закрытия сокета
    /// </summary>
    /// <returns> true - сокет закрыт </returns>
    bool Close()
    {
        if (Socket != INVALID_SOCKET) // если сокет валидный
            if (closesocket(Socket)) // закрываем
                logger.doLog("closesocket fail", GetError());
            else
            {
                Socket = INVALID_SOCKET; // обнуляем дескриптор
                UpdateSockInfo("", 0); // и информацию 
            }

        return Socket == INVALID_SOCKET;
    }
    /// <summary>
    /// назначает внешний адрес, по которому его будут находить транспортные протоколы по 
    /// заданию подключающихся процессов, а также назначает порт, по которому эти 
    /// подключающиеся процессы будут идентифицировать процесс-получатель. 
    /// Предпологается что информация о привязке сокета уже записана.
    /// </summary>
    /// <returns> true - удача, false - неудача </returns>
    bool Bind()
    {
        bool result = false;//результат
        
        if (!IP_port.first.empty() && IP_port.second != 0) // Если задана информация о привязке сокета
        {
            result = (bind(Socket, getSockAddr(), SizeAddr()) == 0); // привязываем его к IP и порту
            if (result) // Логгируем результат
                DEBUG_TRACE(logger, "bind -> ok " + IP_port.first + '.' + std::to_string(IP_port.second))
            else
                logger.doLog("bind -> fail " + IP_port.first + '.' + std::to_string(IP_port.second), GetError());
        }
        else // Если вызвали Bind() не установив информацию о привязке сокета
            logger.doLog("bind invalid IP_port");

        return result;
    }
    /// <summary>
    /// назначает внешний адрес, по которому его будут находить транспортные протоколы по 
    /// заданию подключающихся процессов, а также назначает порт, по которому эти 
    /// подключающиеся процессы будут идентифицировать процесс-получатель.
    /// </summary>
    /// <param name="ip"> - IP адресс в формате "хххх.хххх.хххх.хххх" </param>
    /// <param name="port"> - номер порта </param>
    /// <returns></returns>
    bool Bind(std::string ip, unsigned short port)
    {
        bool result = (checkValidSocet() && setSockInfo(ip, port));
        result = result ? Bind() : false;
        return result;
    }
    /// <summary>
    /// Конструктор с 4-я параметрами
    /// </summary>
    /// <param name="af"> - Семейство адресов: сокеты могут работать с большим семейством адресов. Наиболее частое семейство – IPv4.
    ///  Указывается как AF_INET </param>
    /// <param name="type"> - ип сокета: обычно задается тип транспортного протокола TCP (SOCK_STREAM) или UDP (SOCK_DGRAM). 
    ///  Но бывают и так называемые "сырые" сокеты, функционал которых сам программист определяет в процессе использования.
    ///  Тип обозначается SOCK_RAW </param>
    /// <param name="protocol"> - Тип протокола: необязательный параметр, если тип сокета указан как TCP или UDP – можно передать значение 0. 
    /// </param>
    /// <param name="logger"> - объект для логгирования ошибок </param>
    socket_t(int af, int type, int protocol, log_t& logger) : sockInfo_t(logger), nonBlock(false)
    {
        Socket = socket(af, type, protocol); // привязывает созданный сокет к заданной параметрами транспортной инфраструктуре сети
        checkValidSocet(); // проверяем валидность сокета
    }
    /// <summary>
    ///  Конструктор с 6 параметрами 
    /// </summary>
    /// <param name="af"> - Семейство адресов: сокеты могут работать с большим семейством адресов. Наиболее частое семейство – IPv4.
    ///  Указывается как AF_INET </param>
    /// <param name="type"> - ип сокета: обычно задается тип транспортного протокола TCP (SOCK_STREAM) или UDP (SOCK_DGRAM). 
    ///  Но бывают и так называемые "сырые" сокеты, функционал которых сам программист определяет в процессе использования.
    ///  Тип обозначается SOCK_RAW </param>
    /// <param name="protocol"> - Тип протокола: необязательный параметр, если тип сокета указан как TCP или UDP – можно передать значение 0. 
    /// </param>
    /// <param name="ip"> - IP адресс в формате "хххх.хххх.хххх.хххх" </param>
    /// <param name="port"> - номер порта </param>
    /// <param name="logger"> - объект для логгирования ошибок </param>
    socket_t(int af, int type, int protocol, std::string ip, unsigned short port, log_t& logger) : socket_t(af, type, protocol, logger)
    {
        Bind(ip, port);
    }
    
    /// <summary>
    ///  Конструктор с 5 параметрами 
    /// </summary>
    /// <param name="af"> - Семейство адресов: сокеты могут работать с большим семейством адресов. Наиболее частое семейство – IPv4.
    ///  Указывается как AF_INET </param>
    /// <param name="type"> - ип сокета: обычно задается тип транспортного протокола TCP (SOCK_STREAM) или UDP (SOCK_DGRAM). 
    ///  Но бывают и так называемые "сырые" сокеты, функционал которых сам программист определяет в процессе использования.
    ///  Тип обозначается SOCK_RAW </param>
    /// <param name="protocol"> - Тип протокола: необязательный параметр, если тип сокета указан как TCP или UDP – можно передать значение 0. 
    /// </param>
    /// <param name="sockInfo"> - объект содержащий информацию о сокете
    /// <param name="logger"> - объект для логгирования ошибок </param>
    socket_t(int af, int type, int protocol, sockInfo_t sockInfo, log_t& logger) : socket_t(af, type, protocol, logger)
    {
        checkValidSocet();
        setSockInfo(sockInfo);
        Bind();
    }

    // сокеты - уникальный ресурс, удаляем копирование объектов
    socket_t(const socket_t& sock) = delete;
    socket_t& operator = (const socket_t& sock) = delete;

    /// <summary>
    /// Метод проверки валидности сокета
    /// </summary>
    /// <returns> true - сокет валиден, false - сокет не валиден </returns>
    bool checkValidSocet()
    {
        bool result = Socket != INVALID_SOCKET;

        if(!result) logger.doLog("Socket != INVALID_SOCKET", GetError());

        return result;
    }
    ~socket_t()
    {
        Close();
    }
    /// <summary>
    /// Метод возврата дескриптора сокета
    /// </summary>
    /// <returns> дескриптор сокета </returns>
    SOCKET getSocket() const
    {
        return Socket;
    }
    /// <summary>
    /// установить сокет неблокирующим
    /// </summary>
    /// <param name="NonBlock"></param>
    /// <returns></returns>
    bool setNonBlock()
    {
        if (!nonBlock)
            nonBlock = RAII_OSsock::setSocketOpt(Socket, RAII_OSsock::option_t::NON_BLOCK, logger);
        return nonBlock;
    }
protected:
    SOCKET Socket; // дескриптор сокета
    bool nonBlock; // признак неблокирующего сокета
};
    
    /// <summary>
    /// TCP клиентский сокет 
    /// </summary>
    class TCP_socketClient_t : private socket_t
{
    friend class TCP_socketServer_t; // даем полную свободу серверному сокету (необходимо для acсept())
private :
    /// <summary>
    /// Заменить сокет, данный метод нарушает принцип уникальности ресурса, но необходим для серверного сокета для acсept()
    /// </summary>
    /// <param name="socket"> - новый дескриптор сокета </param>
    /// <param name="sockInfo"> - обновленная информация </param>
    /// <returns> true - успешная замена </returns>
    bool SetSocket(SOCKET socket, sockInfo_t sockInfo)
    {
        bool result = (b_connected = socket_t::SetSocket(socket));
        if (result) serverInfo.setSockInfo(sockInfo);
        return result;
    }
public :
    /// <summary>
    /// Конструткор с 1 параметром 
    /// </summary>
    /// <param name="logger"> - объект для логгирования </param>
    TCP_socketClient_t(log_t& logger) : socket_t(AF_INET, SOCK_STREAM, 0, logger), b_connected(false), serverInfo(logger)
    {}
    /// <summary>
    /// Конструктор с 3 параметрами
    /// </summary>
    /// <param name="ip_server"> - IP адрес сервера в формате "хххх.хххх.хххх.хххх" </param>
    /// <param name="port_server"> - номер порта сервера </param>
    /// <param name="logger"> - объект логгирования </param>
    TCP_socketClient_t(std::string ip_server, unsigned short port_server, log_t& logger) : TCP_socketClient_t(logger)
    {
        if (serverInfo.setSockInfo(ip_server, port_server)) // Если успешно задали информацию о сервере
            Connected(); // устанавливаем соденение с ним
    }
    /// <summary>
    /// Коструктор с 2 параметрами
    /// </summary>
    /// <param name="serverSockInfo"> - информация о сервере </param>
    /// <param name="logger"> - объект логгирования </param>
    TCP_socketClient_t(sockInfo_t serverSockInfo, log_t& logger) : TCP_socketClient_t(logger)
    {
        serverInfo.setSockInfo(serverSockInfo); // задаем информацию о сервере
        Connected(); // устанавливаем соединение
    }
    /// <summary>
    /// Метод приема сообщений с соединенного сокета с поддержкой контроля размера принятого сообщения
    /// </summary>
    /// <param name="str_bufer"> - буфер для приема данных </param>
    /// <param name="str_EndOfMessege"> - строка символов обозначающая конец сообщения (опционально) </param>
    /// <param name="sizeMsg"> - размер ожидаемого сообщения (опционально) </param>
    /// <returns> 0 - сообщение принято поностью; 
    ///           N>0 - принято N байт; 
    ///           -1 - системная ошибка; 
    ///           -2 - соединение закрыто; 
    ///           -3 - данных на входе нет(неблокирующий сокет)</returns>
    int Recive(std::string& str_bufer, const std::string str_EndOfMessege = "", const size_t sizeMsg = 0)
    {
        int result = -1;
        // если есть соединение
        if (b_connected)
        {
            str_bufer.clear(); // готовим буферный параметр
            std::string tempStr(2048, '\0'); // временная строка фиксированного размера для приема данных
            int reciveSize = 0; // размер принятых данных
            bool EOM = str_EndOfMessege.empty() && (sizeMsg == 0); // EndOfMessege признак конца сообщения 
            // цикл приема данных
            do {
                reciveSize = recv(Socket, &tempStr[0], tempStr.size(), 0); // Функция служит для чтения данных из сокета.

                if (reciveSize > 0)
                {// если данные есть
                    DEBUG_TRACE(logger, "Recive msg: " + tempStr)
                    tempStr.assign(tempStr.c_str()); // избовляемся от лишних '\0'; 
                    str_bufer += tempStr; // добавляем в буфер
                    tempStr.clear(); // чистим строку

                    if (!str_EndOfMessege.empty())
                    { // если задан EOM
                        int pos = str_bufer.size() - str_EndOfMessege.size(); // вычисляем позицию ЕОМ в буфуре
                        if (pos >= 0) // если все валидно
                            EOM = (str_bufer.rfind(str_EndOfMessege) == pos); // находим ЕОМ с конца буфера 
                    }
                    if (sizeMsg != 0) // если задан размер сообщения
                        EOM |= (str_bufer.size() >= sizeMsg); // проверяем, не все ли мы уже получили
                    
                    result = EOM ? 0 : reciveSize; // Если приняли все сообщение, то 0, если часть, то кол-во байт
                }
                else if (reciveSize < 0) 
                {   // если сокет не блокирующий, проверяем, может просто нет данных
                    if (nonBlock && GetError() == error_t::NON_BLOCK_SOCKET_NOT_READY) 
                        result = -3; // сокет не блокирующий, нет данных
                    else
                    {   // если была ошибка, логгируем
                        logger.doLog("TCP_socketClient_t::Recive() fail, errno: ", GetError());
                        result = -1; // системная ошибка 
                        b_connected = false; // и разрываем соединение
                    }
                    break;
                }
                else 
                { 
                    result = -2; // соединение закрыто
                    b_connected = false;
                    break;
                }

            } while (!EOM && !nonBlock); // выполнять пока не конец данных, либо мы неблокирующий сокет, и запускаем цикл один раз
        }
        else
            result = -2; // соединение закрыто
         
        return result;
    }
    /// <summary>
    /// Метод отправки сообщений с соединенного сокета с поддержкой контроля размера отправленного сообщения 
    /// </summary>
    /// <param name="str_bufer"> - буфер, содержащий данные для отправки </param>
     /// <returns> 0 - сообщение отправлено поностью; 
    ///           N>0 - отправлено N байт; 
    ///           -1 - системная ошибка; 
    ///           -2 - соединение закрыто; 
    ///           -3 - сокет не готов к отправке (неблокирующий сокет)</returns>
    int Send(const std::string& str_bufer)
    {
        int result = -1;
        // если мы подключены
        if (b_connected)
        {
            int totalSendSize = str_bufer.size(); // требуемое количество отправленных байт
            int sendSize = 0; // текущее количество отправленных байт
            // цикл отправки
            do {
                int tempSize = send(Socket, &str_bufer[sendSize], totalSendSize - sendSize, 0); // используются для пересылки сообщений в другой сокет
                if (tempSize > 0)
                { // Если что то отправили
                    logger.doDebugTrace("send msg: " + std::string(&str_bufer[sendSize], tempSize));
                    sendSize += tempSize;
                    result = (totalSendSize == sendSize) ? 0 : sendSize; // все ли отправили?
                }
                else if (tempSize < 0)
                {// если сокет не блокирующий, проверяем, может просто нет данных
                    if (nonBlock && GetError() == error_t::NON_BLOCK_SOCKET_NOT_READY) 
                        result = -3; // сокет не блокирующий, нет данных
                    else // если ошибка, логгируем ошибку и разрываем соединение, выходим из цикла
                    {
                        logger.doLog("TCP_socketClient_t::Send() fail, errno: ", GetError());
                        result = -1; // системная ошибка
                        b_connected = false; // и разрываем соединение
                    }
                    break;
                }
                else
                {
                    result = str_bufer.empty() ? 0 : -1; // пытались отправить пустое сообщение? у вас получилось
                    break;
                }
            } while ((totalSendSize > sendSize) && !nonBlock);// выполнять пока не все данные, либо мы неблокирующий сокет, и запускаем цикл один раз
        }
        else
            result = -2; // соединение закрыто
        
        return result;
    }
    /// <summary>
    /// Метод подключения сокета к серверному сокету
    /// </summary>
    /// <returns></returns>
    bool Connected()
    {
        bool result = false;
        // если сокет валиден
        if (checkValidSocet())
        {   // функция запускает процесс трехстороннего квитирования - она командует TCP - стеку клиента отправить первый пакет SYN
            if (0 != connect(Socket, serverInfo.getSockAddr(), serverInfo.SizeAddr()))
                logger.doLog("TCP_socketClient_t non connected with server:" + serverInfo.GetIP() + '.' + std::to_string(serverInfo.GetPort()) + " errno: ", GetError());
            else
            {   // если мы успешно подключились, обновляем информацию о себе (нас неявно забиндили)
                result = true;
                UpdateSockInfo();
            }
        }
        b_connected = result;
        
        return result;
    }
    /// <summary>
    /// метод возврата состояния соединения
    /// </summary>
    /// <returns> 1 - соединение есть </returns>
    bool GetConnected() const
    {
        return b_connected;
    }
private:
    bool b_connected; // признак подключения сокета к серверу
    sockInfo_t serverInfo; // информация о сервере
};
    
    /// <summary>
    /// TCP серверный сокет
    /// </summary>
    class TCP_socketServer_t : private socket_t
{
public :
    /// <summary>
    /// конструктор с 3-я параметрами
    /// </summary>
    /// <param name="ip"> - IP адресс в формате "хххх.хххх.хххх.хххх" </param>
    /// <param name="port"> - номер порта </param>
    /// <param name="logger"> - объект логгирования </param>
    TCP_socketServer_t(std::string ip, unsigned short port, log_t& logger) : socket_t(AF_INET, SOCK_STREAM, 0, ip, port, logger)
    { //Функция listen помещает сокет в состояние, в котором он прослушивает входящее соединение
        if (0 != listen(Socket, SOMAXCONN))
            this->logger.doLog("TCP_socketServer_t listen fali ", GetError());
    }
    /// <summary>
    /// конструктор с 2-я параметрами
    /// </summary>
    /// <param name="sockInfo"> - информация о сокете </param>
    /// <param name="logger"> - объект логгирования </param>
    TCP_socketServer_t(sockInfo_t sockInfo, log_t& logger) : socket_t(AF_INET, SOCK_STREAM, 0, sockInfo, logger)
    {
        if (0 != listen(Socket, SOMAXCONN))
            this->logger.doLog("TCP_socketServer_t listen fali ", GetError());
    }
    /// <summary>
    /// метод добавления подключенных клиентов
    /// </summary>
    /// <param name="client"> - ссылка на клиента под замену на подключенного клиента </param>
    /// <returns> 0 - добавлен подключенный клиент,
    ///          -1 - системная ошибка, 
    ///          -2 - нет клиентов в очереди на подключение (неблокирующий сокет)</returns> 
    int AddClient(TCP_socketClient_t& client)
    {
        int result = -1;

        sockInfo_t tempInfo(logger); // информация о подключенном сокете
        int sizeAddr = tempInfo.SizeAddr(); // ее размер
        //функция используется сервером для принятия связи на сокет. Сокет должен быть уже слушающим в момент вызова функции. 
        //Если сервер устанавливает связь с клиентом, то функция accept возвращает новый сокет-дескриптор, через который 
        //и происходит общение клиента с сервером.
        SOCKET tempSocket = accept(Socket, tempInfo.setSockAddr(), &sizeAddr);
        if (tempSocket > -1)
        { // если есть какой то сокет, то устанавливаем его
            tempInfo.UpdateSockInfo(); // предварительнообновив информацию о сокете
            if (client.SetSocket(tempSocket, tempInfo))
            { // все получилось? вывод отладки и результата
                DEBUG_TRACE(logger, "addClient success" + tempInfo.GetIP() + std::to_string(tempInfo.GetPort()))
                result = 0;
            }
            else // иначе проблема в установке сокета
                logger.doLog("fail SetSocket in addClient", GetError());
        } // или мы не получили никакой сокет и если мы неблокирующий сокет и ошибка связана с отсутствием клиентов в очереди на подключение
        else if (GetError() == error_t::NON_BLOCK_SOCKET_NOT_READY && nonBlock)
            result = -2;
        else
            logger.doLog("accept fail", GetError()); // иначе это системная ошибка
        
        return result;
    }
};
    
    /// <summary>
    /// UDP сокет
    /// </summary>
    class UDP_socket_t : private socket_t
{
protected :
    /// <summary>
    /// метод установки MTU
    /// </summary>
    void setMTU()
    {
        int optlen = sizeof(u32_MTU); // размер опции
        // Функция getsockopt извлекает текущее значение для параметра сокета, связанного с сокетом любого типа, в любом состоянии
        if (getsockopt(Socket, SOL_SOCKET, SO_MAX_MSG_SIZE, (char*)(&u32_MTU), &optlen))
            logger.doLog("getsockopt fail ", GetError());
    }
public :
    /// <summary>
    /// Конструткор с 1 параметром 
    /// </summary>
    /// <param name="logger"> - объект для логгирования </param>
    UDP_socket_t(log_t& logger) : socket_t(AF_INET, SOCK_DGRAM, 0, logger), lastCommunicationSocket(logger)
    {
        setMTU();
    }
    /// <summary>
    /// конструктор с 3-я параметрами
    /// </summary>
    /// <param name="ip"> - IP адресс в формате "хххх.хххх.хххх.хххх" </param>
    /// <param name="port"> - номер порта </param>
    /// <param name="logger"> - объект логгирования </param>
    UDP_socket_t(std::string ip, unsigned short port, log_t& logger) : socket_t(AF_INET, SOCK_DGRAM, 0, ip, port, logger), lastCommunicationSocket(logger)
    {
        setMTU();
    }
    /// <summary>
    /// конструктор с 2-я параметрами
    /// </summary>
    /// <param name="sockInfo"> - информация о сокете </param>
    /// <param name="logger"> - объект логгирования </param>
    UDP_socket_t(sockInfo_t& sockInfo, log_t& logger) : socket_t(AF_INET, SOCK_DGRAM, 0, sockInfo, logger), lastCommunicationSocket(logger)
    {
        setMTU();
    }
    /// <summary>
    /// Метод отправки данных в сеть без предварительного соединения
    /// </summary>
    /// <param name="buffer"> - область с данными для отправки </param>
    /// <param name="target"> - информация о сокете приемнике </param>
    /// <returns> 0 - отправлено все сообщение; 
    ///         N>0 - отправлено N байт; 
    ///          -1 - системная ошибка;
    ///          -2 - размер сообщения больше MTU 
    ///          -3 - сокет не готов к отправке (неблокирующий сокет) </returns>
    int SendTo(const std::string& buffer, sockInfo_t target)
    {
        int result = -1;
        // проверяем размер сообщения
        if (buffer.size() < MTU())
        { // Функция sendto отправляет данные в определенное место назначения
            int sendSize = sendto(Socket, buffer.c_str(), buffer.size(), 0, target.getSockAddr(), target.SizeAddr());
            // проверяем результат
            if (sendSize > 0)
            { // если есть положительный результат
                result = (sendSize == buffer.size()) ? 0 : sendSize; // если сообщение отправилось полностью, то 0 - все гуд, если нет, то количество переданых байт
                DEBUG_TRACE(logger, "sendto: " + buffer)
            }
            else if (sendSize < 0)
            { // если есть ошибка, проверяем, связана ли она с блокировкой при неблокирующем сокете
                if (GetError() == error_t::NON_BLOCK_SOCKET_NOT_READY && nonBlock)
                    result = -3; // сокет не готов (неблокирующий)
                else
                    logger.doLog("sendto fail ", GetError()); // иначе системная ошибка
            }
            else if (buffer.empty()) // если пытались отправить ноль? то у нас все получилось
                result = 0;
        }
        else
            result = -2; // размер сообщения больше MTU

        // если отправили данные новому сокету, обновляем информацию об этом (как последний набранный номер)
        if (target != lastCommunicationSocket)
            lastCommunicationSocket.setSockInfo(target);

        return result;
    }
    /// <summary>
    /// Метод отправки данных в сеть без предварительного соединения, отправка предыдущему сокету с которым было взаимодействие
    /// </summary>
    /// <param name="buffer"> - область с данными для отправки </param>
    /// <returns> 0 - отправлено все сообщение; 
    ///         N>0 - отправлено N байт; 
    ///          -1 - системная ошибка;
    ///          -2 - размер сообщения больше MTU 
    ///          -3 - сокет не готов к отправке (неблокирующий сокет) </returns>
    int SendTo(const std::string& buffer)
    {
        return SendTo(buffer, lastCommunicationSocket); // отправка предыдущему сокету с которым было взаимодействие
    }
    /// <summary>
    /// Метод отправки данных в сеть без предварительного соединения
    /// </summary>
    /// <param name="buffer"> - область с данными для отправки </param>
    /// <param name="ip"> - IP адресс в формате "хххх.хххх.хххх.хххх" </param>
    /// <param name="port"> - номер порта </param>
    /// <returns> 0 - отправлено все сообщение; 
    ///         N>0 - отправлено N байт; 
    ///          -1 - системная ошибка;
    ///          -2 - размер сообщения больше MTU 
    ///          -3 - сокет не готов к отправке (неблокирующий сокет) </returns>
    int SendTo(const std::string& buffer, std::string ip, unsigned short port)
    {
        if (lastCommunicationSocket.setSockInfo(ip, port)) // задаем информацию о сокете, если все успешно
            return SendTo(buffer); // оправляем
        else return -1;
    }
    /// <summary>
    /// Метод приема данных из сети без предварительного соединения
    /// </summary>
    /// <param name="buffer"> - буфер с принятыми данными </param>
    /// <param name="str_EndOfMessege"> - признак конца сообщения (опционально) </param>
    /// <param name="sizeMsg"> - ожидаемы размер сообщения (опционально) </param>
    /// <returns>   0 - все сообщение принято успешно; 
    ///             N>0 - принято N-байт;
    ///             -1 - системная ошибка;
    ///             -2 - соеднинение закрыто;
    ///             -3 - сокет не готов (неблокирующий);</returns>
    int RecvFrom(std::string& buffer, const std::string str_EndOfMessege = "", const size_t sizeMsg = 0)
    {
        int result = -1;

        buffer.clear(); // готовим буфер 
        std::string tempStr(2048, '\0'); // временная строка фиксированного размера для приема данных
        int SizeAddr = lastCommunicationSocket.SizeAddr(); // размер структуры Addr
        // Функция recvfrom получает датаграмму и сохраняет исходный адрес
        int recvSize = recvfrom(Socket, &tempStr[0], tempStr.size(), 0, lastCommunicationSocket.setSockAddr(), &SizeAddr);

        if (recvSize > 0)
        { // если результат положителен
            buffer.assign(tempStr.c_str()); // убираем лишние '\0' в конце строки 
            DEBUG_TRACE(logger, "recvfrom: " + tempStr) 

            bool EOM = str_EndOfMessege.empty() && (sizeMsg == 0);// EndOfMessege признак конца сообщения
            if (!str_EndOfMessege.empty())
            { // если задан EOM
                int pos = buffer.size() - str_EndOfMessege.size(); // вычисляем позицию ЕОМ в буфуре
                if (pos >= 0) // если все валидно
                    EOM = (buffer.rfind(str_EndOfMessege) == pos); // находим ЕОМ с конца буфера 
            }
            if (sizeMsg != 0) // если задан размер сообщения
                EOM |= (buffer.size() >= sizeMsg); // проверяем, не все ли мы уже получили

            result = EOM ? 0 : recvSize; // если получили все, то 0, иначе кол-во полученных байт

            lastCommunicationSocket.UpdateSockInfo(); // вызвваем после перезаписи setSockAddr() 
        }
        else if (recvSize < 0)
        { // если появилась ошибка, проверяем не связана ли она с блокировкой неблокирующего сокета 
            if (GetError() == error_t::NON_BLOCK_SOCKET_NOT_READY && nonBlock)
                result = -3;
            else
                logger.doLog("recfrom fail ", GetError());
        }
        else
            result = -2; // соединение закрыто

        return result;
    }
    /// <summary>
    /// метод возврата информации о сокете с которым происходило последнее взаимодействие (отправка/прием данных)
    /// </summary>
    /// <returns> сокет с которым происходило последнее взаимодействие (отправка/прием данных) </returns>
    sockInfo_t GetLastCommunication() const
    {
        return lastCommunicationSocket;
    }
    /// <summary>
    /// метод возврата MTU
    /// </summary>
    /// <returns> максимальный размер сообщения </returns>
    unsigned int MTU() const
    {
        return u32_MTU;
    }
private:
    sockInfo_t lastCommunicationSocket; // Последний сокет, с кем происходило взаимодействие
    unsigned int u32_MTU; // максимальный размер отправляемых данных
};
    
    /// <summary>
    /// Класс мультиплексирования неблокирующих сокетов
    /// </summary>
    class NonBlockSocket_manager_t : private RAII_OSsock
{
protected : 
    /// <summary>
    /// метод добавления сокета в один из списков
    /// </summary>
    /// <param name="m_sock"> - сокет на добавление </param>
    /// <param name="socket"> - ассоциативный массив для добавления </param>
    /// <returns> 1 - сокет добавлен </returns>
    bool addSocket(std::map<int, socket_t&>& m_sock, socket_t& socket)
    {
        bool result = false;
        
        if (socket.checkValidSocet()) // сокет валидный?
            if (socket.setNonBlock()) // если сокет неблокирующий
                if (result = m_sock.find(socket.getSocket()) == m_sock.end()) // и его еще нет в списках
                {
                    m_sock.emplace(socket.getSocket(), socket); // добавляем его 
                    b_change = true; // произошли изменения, нужно менять pollfd
                }
        
        return result;
    }
    /// <summary>
    /// Метод удаления сокета из списков
    /// </summary>
    /// <param name="m_sock"> - сокет на удаление </param>
    /// <param name="socket"> - ассоциативный массив для добавления </param>
    /// <returns> 1 - сокет удален </returns>
    bool deleteSocket(std::map<int, socket_t&>& m_sock, socket_t& socket)
    {
        bool result = false;
        // нужно ли делать сокет блокирующим после удаления? по идее нет, удалили от сюда сокет, можем его добавить в другой менеджер
        if (result = m_sock.find(socket.getSocket()) != m_sock.end()) // если такой сокет есть в списке
        {
            m_sock.erase(socket.getSocket()); // удаляем его
            b_change = true; // произошли изменения, нужно менять pollfd
        }
        
        return result;
    }
    /// <summary>
    /// Метод обновления структуры pollfd
    /// </summary>
    void UpdatePollfd()
    {
        if (b_change)
        { // если были изменения в списках сокетов
            b_change = false;
            size_t size = m_senderSocket.size() + m_readerSocket.size(); // размер массива структур pollfd
            v_fds.clear(); // готовим массив структур pollfd
            v_fds.resize(size);

            size_t indx = 0; // индекс для обхода массива структур pollfd
            std::map<int, socket_t&>::const_iterator iter_sender = m_senderSocket.cbegin(); // итератор для обхода отправителей
            std::map<int, socket_t&>::const_iterator iter_reader = m_readerSocket.cbegin(); // итератор для обхода читателей
            // цикл для обхода массива структур pollfd
            for (; indx < size; ++indx)
            {
                if (iter_sender != m_senderSocket.cend()) // заносим отправителей
                {
                    v_fds[indx].fd = iter_sender->second.getSocket(); // дескриптор сокета
                    v_fds[indx].events = POLLOUT; // ожидаемое событие
                    v_fds[indx].revents = 0; // произошедшее событие
                    ++iter_sender;
                }
                else if (iter_reader != m_readerSocket.cend()) // заносим читателей
                {
                    v_fds[indx].fd = iter_reader->second.getSocket();
                    v_fds[indx].events = POLLIN;
                    v_fds[indx].revents = 0;
                    ++iter_reader;
                }
                else break; // если списки кончились, выходим (не должно произойти)
            }

        } // если изменений не было, просто обнуляем произошедшие события
        else for (auto iter : v_fds) iter.revents = 0; 

    }
    /// <summary>
    /// функция мультиплексированя неблокирующих сокетов
    /// </summary>
    /// <param name="timeOut"> - время timeout для функции мультиплексирования </param>
    /// <returns> -1 - системная ошибка; 0 - вышел таймаут и событий не произошло; N>0 - кол-во событий </returns>
    int Poll(int timeOut)
    {
#ifdef __WIN32__
        return WSAPoll(&v_fds[0], v_fds.size(), timeOut);//Функция WSAPoll возвращает состояние сокета в элементе revents структуры WSAPOLLFD
#else
        return poll(&v_fds[0], v_fds.size(), timeOut);
#endif
    }
public :
    /// <summary>
    /// Конструктор с одним параметром
    /// </summary>
    /// <param name="logger"> - объект для логиирования </param>
    NonBlockSocket_manager_t(log_t& logger) : RAII_OSsock(logger), b_change(false), logger(logger)
    {}
    /// <summary>
    /// Конструктор с двумя параметрами
    /// </summary>
    /// <param name="size"> - предварительное количество наблюдаемых сокетов </param>
    /// <param name="logger"> - объект для логирования </param>
    NonBlockSocket_manager_t(int size, log_t& logger) : NonBlockSocket_manager_t(logger)
    {
        v_fds.reserve(size);
    }
    /// <summary>
    /// метод добавления отправителя
    /// </summary>
    /// <param name="socket"> - целевой сокет </param>
    /// <returns> 1 - сокет добавлен </returns>
    bool AddSender(socket_t& socket) // добавляем сокет 
    {
        return addSocket(m_senderSocket, socket);
    }
    /// <summary>
    /// метод удаления отправителя
    /// </summary>
    /// <param name="socket"> - целевой сокет </param>
    /// <returns> 1 - сокет удален </returns>
    bool deleteSender(socket_t& socket)
    {
        return deleteSocket(m_senderSocket, socket);
    }
    /// <summary>
    /// метод добавления читателя
    /// </summary>
    /// <param name="socket"> - целевой сокет </param>
    /// <returns> 1 - сокет добавлен </returns>
    bool AddReader(socket_t& socket)
    {
        return addSocket(m_readerSocket, socket);
    }
    /// <summary>
    /// метод удаления читателя
    /// </summary>
    /// <param name="socket"> - целевой сокет </param>
    /// <returns> 1 - сокет удален </returns>
    bool deleteReader(socket_t& socket)
    {
        return deleteSocket(m_readerSocket, socket);
    }
    /// <summary>
    /// основной метод работы мультиплесора 
    /// </summary>
    /// <param name="senderReady"> - список сокетов получивших событие на отправление </param>
    /// <param name="readerReady"> - список сокетов получивших событие на прием </param>
    /// <param name="timeOut"> - время ожидания мультиплесора </param>
    /// <returns> 1 - принято хотябы одно событие </returns>
    bool Work(std::list<socket_t&>& senderReady, std::list<socket_t&>& readerReady, const int timeOut)
    {// подготовка списков
        senderReady.clear(); 
        readerReady.clear();
        // обновление структуры pollfd
        UpdatePollfd();
        // вызываем фукцию мультеплексирования
        int resPoll = Poll(timeOut); 
        if (resPoll > 0) // если результат положителен
            for (auto iter : v_fds)
            { // по все мтруктурам pollfd
                if (iter.revents & POLLIN) // нашли событие на получение
                    readerReady.push_back(m_readerSocket[iter.fd]); // добавляем в список
                if (iter.revents & POLLOUT) // нашли событие на отправление
                    senderReady.push_back(m_senderSocket[iter.fd]); // добавляем в список
            }
        else if (resPoll < 0) // системная ошибка
            logger.doLog("poll error", GetError());
        
        return !senderReady.empty() || !readerReady.empty(); // хоть что то добавили в списки? 
    }
protected :
    std::vector <struct pollfd> v_fds; // динамический массив структур pollfd
    std::map<int, socket_t&> m_senderSocket; // список сокетов ожидающих отправку
    std::map<int, socket_t&> m_readerSocket; // список сокетов ожидающих прием
    bool b_change; // флаг изменения структур pollfd
    log_t& logger; // объект логгирования
};
};

#define INVALID_TASK  -1; // невалидный таск
typedef int taskID; // номер таски

/// <summary>
/// Абстрактный класс функтора 
/// </summary>
class ABStask
{
public: 
    /// <summary>
    /// Основной метод работы
    /// </summary>
    /// <param name="stop"> - при получении данного признака, необходимо завершить работу метода </param>
    virtual void Work (const std::atomic<bool>& stop) = 0;
    virtual ~ABStask() {} 
};

/// <summary>
/// Класс пулл потоков
/// </summary>
class poolThread_t
{
protected :
    /// <summary>
    /// композиционный класс таск
    /// </summary>
    class task_t
    {
    private :
        /// <summary>
        /// Метод возведения флага активности
        /// </summary>
        /// <param name="active"> - целевое значение </param>
        void SetActive(bool active)
        {// если изменения валидны 
            if (active != b_active)
            {  // блокируем мьютекс
                std::lock_guard<std::mutex> lock(mutex);
                b_active = active; // и устанавливаем значение
                if (b_active) // если таска стала активной
                    cv_condition.notify_one(); // толкаем условную переменную
            }
        }
    public:
        /// <summary>
        /// Конструктор по умолчанию
        /// </summary>
        task_t() : b_stop(false), b_active(false), p_userTask(nullptr)
        {}
        /// <summary>
        /// Метод основной работы. Запускается в другом потоке
        /// </summary>
        void Work()
        {
            while (1) 
            {
                std::unique_lock<std::mutex> uniqlock(mutex); // получить блокировку того же мьютекса, который используется для защиты общей переменной проверить, что необходимое условие ещё не выпонлено;
                cv_condition.wait(uniqlock, [this] { return GetActive(); });// Операции ожидания освобождают мьютекс и приостанавливают выполнение потока;
                // когда получено уведомление, истёк тайм - аут или произошло ложное пробуждение, поток пробуждается, и мьютекс повторно блокируется.Затем поток должен проверить, что условие, действительно, выполнено, и возобновить ожидание, если пробуждение было ложным.
                // Вместо трёх последних шагов можно воспользоваться перегрузкой методов wait, wait_for и wait_until, которая принимает предикат для проверки условия и выполняет три последних шага.;
                if (!b_stop) // если нет остановки потоков
                {
                    if (p_userTask != nullptr) // проверка валидности указателя
                        p_userTask->Work(b_stop); // запускаем пользовательскую таску
                    SetActive(false); // после пользовательской таски, отдыхаем
                    p_userTask == nullptr; // затираем выполненую таску
                }
                else
                    return; // конец работы? выходим
            }
        }
        /// <summary>
        /// Метод добавления таски
        /// </summary>
        /// <param name="userTask"> - указатель на объект пользовательской таски </param>
        /// <returns> 1 - таска обновлена </returns>
        bool UpdateTask(std::shared_ptr<ABStask> userTask) 
        {
            if (!GetActive()) // если таска была не активной
            {
                this->p_userTask = userTask; // задаем таску
                SetActive(true); // таска тепрь активна
                return true; 
            }
            else return false;
        }
        /// <summary>
        /// Метод проверки активности
        /// </summary>
        /// <returns> 1 - таска активна </returns>
        bool GetActive() const
        {   
            std::lock_guard<std::mutex> lock(mutex);
            return b_active; 
        }
        /// <summary>
        /// Метод окончательного выхода из таски 
        /// </summary>
        void Stop()
        {
            b_stop = true; // возвести флаг на остановку
            if (!GetActive()) // если таска не активна
                SetActive(true); // будим ее
        }
    protected :
        std::condition_variable cv_condition; // условная переменная
        mutable std::mutex mutex; // мьютекс для условной переменной // ТОДО а нужен ли мьютикс для атомик?
        std::atomic<bool> b_stop; // флаг останова данной таски 
        bool b_active; // флаг активности данной таски

        std::shared_ptr<ABStask> p_userTask; // указатель на пользовательский таск
    };
    /// <summary>
    /// Структура реализующая взаимодействие потока с таской  
    /// </summary>
    struct thread_t
    {
        std::thread thread; // стандартный поток
        task_t task; // таска
        /// <summary>
        /// конструктор по умолчанию
        /// </summary>
        thread_t() : thread([this]() { task.Work(); }) // передаем метод Work() таски для работы в отдельном потоке
        {}
        ///деструктор
        ~thread_t()
        {
            task.Stop(); // тормозим таску 
            thread.join(); // ждем окончания потока
        }
    };

public:
    /// <summary>
    /// Конструктор с одним параметром
    /// </summary>
    /// <param name="size"> - размер пулла потоков </param>
    poolThread_t(size_t size) : counter(0)
    {
        v_thread.resize(size); // перевыделяем под новый размер
        v_taskID.resize(size);
    }
    ~poolThread_t()
    {
        v_thread.clear();
        v_taskID.clear();
    }
    /// <summary>
    /// Мето обновления таски
    /// </summary>
    /// <param name="userTask"> - указатель на объект пользовательской таски </param>
    /// <returns> INVALID_TASK - нет свободных потоков; ID>INVALID_TASK - ID таски </returns>
    taskID UpdateTask(std::shared_ptr<ABStask> userTask)
    {
        taskID result = INVALID_TASK; // результат

        size_t size = v_thread.size();
        for (size_t index = 0; index < size; ++index) // ищем свободную таску
            if (!v_thread[index].task.GetActive()) 
                if (v_thread[index].task.UpdateTask(userTask)) // пробуем задать её
                {
                    result = ++counter; // задаем ID автоинкрементом
                    v_taskID[index] = result; // сохраняем ID
                }
        
        return result;
    }
    /// <summary>
    /// Метод проверки активности таски
    /// </summary>
    /// <param name="ID"> - ID целевой таски </param>
    /// <returns> 1 - таска активна </returns>
    bool GetActiveTask(taskID ID)
    {
        size_t size = v_thread.size();
        for (size_t index = 0; index < size; ++index)
            if (v_taskID[index] == ID)
                return v_thread[index].task.GetActive();

        return false;
    }
protected :
    std::vector<thread_t> v_thread; // вектор потоков
    std::vector<taskID> v_taskID; // вектор ID тасков 
    taskID counter; // автоинкремент 
};

// заглушка
class step_t 
{
public: 
    bool operator == (const step_t& rvalue) const; 
    bool fromString(const std::string& stirng);
    std::string toString() const;
};

/// <summary>
/// Простая база данных для хранения ходов и их стоимости
/// </summary>
class dataBase_t
{
protected :
    /// <summary>
    /// Метод разбивки строки
    /// </summary>
    /// <param name="target"> - целевая строка, в которую записывается ответ, либо, при неудаче, она форматируется </param>
    /// <param name="source"> - исходная строка </param>
    /// <param name="curPos"> - текущая позиция в исходной строке, до которой поиск уже был осуществлен </param>
    /// <param name="delimiter"> - строка разделитель(опционально) </param>
    /// <returns> 1- целевая строка получена </returns>
    bool split(std::string& target, const std::string& source, size_t& curPos, const std::string delimiter = " ")
    {
        target.clear(); // чистим целевую строку
        
        if (curPos <= source.size()) // если позиция валидна
        {
            size_t newPos = source.find(delimiter, curPos); // ищем разделитель в исходной строке с текущей позиции
            if (newPos != std::string::npos) // если что то есть
            {
                target = source.substr(curPos, newPos); // копируем промежуто от текущей позиции, до новой, недавно найденой
                curPos = newPos + delimiter.size(); // меняем текущюю позицию
                DEBUG_TRACE(logger, "split: " + target + ' ')
            }
        }
        else
            logger.doLog("invalid pos in split() "); // позиция, при правильной работе, должна быть валидна

        return !target.empty(); // что нибудь скопировали?
    }
    /// <summary>
    /// Метод парсинга строки в базу данных.
    /// формат строки: "состояние_поля::ход==стоимость;;ход==стоимость;; ... ... "
    /// </summary>
    /// <param name="line"> - исходная строка </param>
    /// <returns> 1 - строка распознана </returns>
    bool parseLine(const std::string& line)
    {
        bool result = false;

        unsigned long long stateField = 0; // индекс для мапа базы данных (состояние игрового поля)
        step_t tempStep; // временный шаг для парсинга
        int stepPrice = 0; // временная цена шага для парсинга
        
        size_t countWord = 0; // номер текущего слово
        size_t pos = 0; // текущая позиция в строке
        std::string word; // отделенное слово

        do { // основной цикл метода с постусловием
            switch (++countWord)
            { // если слово первое - это состояние игрового поля (индекс мапа БД) 
            case 1 :
                if (split(word, line, pos, "::")) // пробуем отделить его,
                {
                    try 
                    { // слово отделили, теперь пробуем распарсить его в индекс
                        stateField = std::stoull(word);
                        result = true;
                    }
                    catch (const std::exception& e)
                    { // поймали исключение? дальше нет смысла смотреть все остальное
                        logger.doLog("parseLine err in case 1 : " + std::string(e.what()));
                        result = false;
                        word.clear();
                    }
                }
                else
                    logger.doLog("parseLine empty line"); // строка была пуста
                break;
            case 2 : // если слово второе - это ход
                if (split(word, line, pos, "=="))
                { // если отделили и распарсили его, то ок
                    if (!tempStep.fromString(word)) 
                        result = false;
                }
                else
                {
                    result = false;
                    logger.doLog("parseLine non full line"); // строка неполная
                }
                break;
            case 3 : // а это цена хода
                if (split(word, line, pos, ";;"))
                { // 0 - невалидная стоимость, либо отрицательная - очень дешевый и плохой ход, либо положительный - дорогой и хороший ход
                    if (stepPrice = std::atol(word.c_str()))
                        result = false;
                }
                else
                    result = false;

                if (result) // если имеется 3 слова + результат положителен
                {
                    countWord = 1; // сбрасываем кол-во слов до 1, индекс то один на строку, а вот ходов со стоимостью, неограничено
                    if (!getStepPrice (stateField, tempStep)) // если такого хода в базе еще нет
                        data[stateField].push_back(std::make_pair(tempStep, stepPrice)); // добавляем
                    else
                    {   // иначе, ищем его
                        std::list<std::pair <step_t, int> >::iterator iter = data[stateField].begin(); // начальный итератор
                        std::list<std::pair <step_t, int> >::iterator end = data[stateField].end(); // конечный итератор
                        for (; iter != end; ++iter) // по всему списку
                            if (iter->first == tempStep) // если ходы равны
                                iter->second = stepPrice; // изменяем стоимость
                    }
                }


            default: // куда то вышли не туда
                logger.doLog("parseLine come to default");
                result = false; // все чистим
                word.clear(); 
                break;
            }
        } while(!word.empty()); // крутимся пока находим слова
        
        return result;
    }

    /// <summary>
    /// Метод загрузки данных из файла в БД
    /// </summary>
    /// <param name="fileName"> - имя файла для загрузки (опционально)</param>
    /// <returns> pair.first - количество распознанных строк; pair.second - количество нераспознанных строк </returns>
    std::pair<size_t, size_t> loadData(std::string fileName = "")
    {
        std::pair<size_t, size_t> result;

        std::ifstream ifile; // открываем файл, имя выбираем в зависимости от наличия параметра в методе
        ifile.open( fileName.empty() ? this->fileName : fileName );
        if (ifile.is_open())
        {
            size_t counter = 0, err = 0; // счетчики распознанных и нераспознанных строк
            std::string line;
            while (std::getline(ifile, line)) // пока можем считать линию
                if (parseLine(line)) // парсим ее
                    ++counter; // смогли распарсить
                else
                    ++err; // не смогли
            // формируем результат
            result.first = counter;
            result.second = err;
            // закрываем файл
            ifile.close();
        }
        else
            logger.doLog("file: " + (fileName.empty() ? this->fileName : fileName) + " fail open");

        return result;
    }
    /// <summary>
    /// Метод сохранения данных из БД в файл
    /// </summary>
    /// <returns> 1 - данный сохранены </returns>
    bool saveData()
    {
        bool result = false;

        std::ofstream ofile;
        ofile.open(fileName); // открываем файл
        if (ofile)
        {
            for (auto iterMap : data) // весь мап
            { // записываем в файл
                ofile << iterMap.first << "::"; // индекс мапа (состояние игрового поля)
                for (auto iterList : iterMap.second) // весь список ходов и стоимостей
                    ofile << iterList.first.toString() << "==" << iterList.second << ";;"; // записываем в файл 

                ofile << '\n';
            }

            result = true; 
            ofile.close();
        }
        else
            logger.doLog("file: " + fileName + " fail open");

        return result;
    }
public :
    /// <summary>
    /// конструктор с 2-я параметрами
    /// </summary>
    /// <param name="fileName"> - имя файла для загрузки данных </param>
    /// <param name="logger"> - объект логгирования</param>
    dataBase_t(std::string fileName, log_t& logger) : fileName(fileName), logger(logger)
    {
        loadData();
    }
    ~dataBase_t()
    {
        saveData();
    }
    /// <summary>
    /// Метод возврата стоимости хода, чем дороже ход, тем он лучше (ценнее)
    /// </summary>
    /// <param name="stateField"> - текущее состояние игрового поля</param>
    /// <param name="step"> - целевой шаг (ход)</param>
    /// <returns> 0 - такого хода не в БД; !=0 - стоимость хода </returns>
    int getStepPrice(unsigned long long stateField, step_t step) const
    {

        std::map<unsigned long long, std::list<std::pair <step_t, int> > > ::const_iterator iterMap = data.find(stateField);
        if (iterMap != data.cend())
        {
            std::list<std::pair <step_t, int> >::const_iterator iterList = iterMap->second.cbegin();
            std::list<std::pair <step_t, int> >::const_iterator iterEnd = iterMap->second.cend();
            for (; iterList != iterEnd; ++iterList)
                if (iterList->first == step)
                    return iterList->second;
        }

        return 0;
    }
    /// <summary>
    /// Метод обновления стоимости хода, если комбинация была выигрышная - ход дорожает, иначе - дешевеет
    /// </summary>
    /// <param name="stateField"> - текущее состояние игрового поля</param>
    /// <param name="step"> - целевой шаг (ход)</param>
    /// <param name="winner"> - 1 - комбинация была победной </param>
    void UpdateStepPrice(unsigned long long stateField, step_t step, bool winner)
    {
        std::map < unsigned long long, std::list<std::pair <step_t, int> > > ::iterator iterMap = data.find(stateField); // находим есть ли запись на заданное состояние поля 
        if (iterMap != data.cend())
        { // если есть
            std::list<std::pair <step_t, int> >::iterator iterList = iterMap->second.begin(); 
            std::list<std::pair <step_t, int> >::iterator iterEnd = iterMap->second.end();
            for (; iterList != iterEnd; ++iterList) // смотрим весь список ходов и стоимостей
                if (iterList->first == step) // если нашли соответствие
                {   
                    do {
                        if (winner) // выигрышный ход?
                            ++iterList->second; // ход дорожает
                        else
                            --iterList->second; // дешевеет
                    } while (iterList->second == 0); // стоимость не должна быть равна нулю

                    return; // выход, дальше искать смысла нет
                }
            // если не нашли ход=стоимость, добавляем 
            iterMap->second.push_back(std::make_pair(step, winner ? 1 : -1));
        }
        else // если нет записи на заданное состояние - добавляем
            data[stateField].push_back(std::make_pair(step, winner ? 1 : -1));
    }
    /// <summary>
    /// Метод обновления базы данных
    /// </summary>
    /// <param name="fileSource"> - имя файла для обновления </param>
    /// <returns> 1 - обновление прошло успешно </returns>
    bool UpdateDataBase(std::string fileSource)
    {
        std::pair<size_t, size_t> result;

        if (!fileSource.empty()) 
            result = loadData(fileSource); // обновляемся

        return result.first > 0 && result.second == 0; // если что то обновили и без ошибок
    }
protected : 
    log_t& logger; // объект логирования
    std::string fileName; // имя файла хранения данных для БД
    // БД в формате ассоциативного массива, где ключ - уникальное состояние поля;
    // значение - список пар где первое значение - ход, второе - стоимость хода
    std::map <unsigned long long, std::list<std::pair <step_t, int> > > data; 
};

/// <summary>
/// /////////////////////////////////
/// </summary>

namespace graphics2D
{
    /*
     private :
        inline void SinglUpdateLight(unsigned char color, int stepLight)
        {
            color = color + stepLight > 255 ? 255 :
                color + stepLight < 0 ? 0 : color + stepLight;
        }
            void UpdateLight(int stepLight)
        {
            SinglUpdateLight(R, stepLight);
            SinglUpdateLight(G, stepLight);
            SinglUpdateLight(B, stepLight);
        }
        void Move(direction_t direct, int step = 1)
        {
            switch (direct)
            {
            case left :
                X -= step;
                break;
            case leftUp :
                X -= step;
                Y -= step;
                break;
            case up :
                Y -= step;
                break;
            case upRight :
                X += step;
                Y -= step;
                break;
            case right :
                X += step;
                break;
            case rightDown :
                X += step;
                Y += step;
                break;
            case down :
                Y += step;
                break;
            case downLeft :
                X -= step;
                Y += step;
                break;
            default:
                break;
            }
        }
                virtual bool Move(direction_t direct, int step = 1) = 0;
        bool Move(int step = 1)
        {
            return Move(direct, step);
        }

    */
    enum direction_t
    {
        left, leftUp, up, upRight, right, rightDown, down, downLeft 
    };
    
    struct color_t // цвет объектов в окне
    {
        unsigned char R/*красный*/, G/*зеленый*/, B/*синий*/;
        color_t(unsigned char red, unsigned char green, unsigned char blue) : R(red), G(green), B(blue)
        {}
        color_t() : R(0), G(0), B(0)
        {}
        operator unsigned()
        {
            return static_cast<unsigned>(B) + static_cast<unsigned>(G << 8) + static_cast<unsigned>(R << 16);
        }
    };

    struct coord_t
    {
        short X, Y;
        coord_t(short X, short Y) : X(X), Y(Y)
        {}
        coord_t() : X(0), Y(0)
        {}
        bool operator < (coord_t& rValue)
        {
            return Y < rValue.Y ? true : Y == rValue.Y && X < rValue.X;
        }
        bool operator == (coord_t& rValue)
        {
            return Y == rValue.Y && X == rValue.X;
        }
    };

    class bitmap_t
    {
    protected :
        std::map <coord_t, std::vector<color_t> > screen;

    };

    class window_t
    {
        
    };

    class figureABS
    {
    public :
        void Update(coord_t coord)
        {
            ancor = coord;
        }
        void Update(color_t color)
        {
            this->color = color;
        }
        void Update(direction_t direct)
        {
            this->direct = direct;
        }


    protected :
        std::vector<coord_t> v_coord;
        coord_t ancor;
        color_t color;
        direction_t direct;
        window_t& window;
    };

    class square_t;

    class circle_t;

    class triangle_t;

    class rectangle_t;

    class font_t;
};

class crypto_t
{
    bool encrypt(char* dst, const char* src, int len);
    bool decipher(char* dst, const char* src, int len);
};

class clientSsesion_t
{
public :
    bool send(const char* buf, int len);
    bool recive(char* buf, int len);
};
class packer_t
{
public :
    int compression(const std::string& stateField, std::string possiblStep, std::string step, char* buf, int len)
    {
        int result = 0;
        return result;
    }
    int deCompression(std::string& stateField, std::string& possiblStep, std::string& step, const char* buf, int len)
    {
        int result = 0;
        return result;
    }
};


////////////////////h
constexpr auto STATE_INIT_VALUE = (signed int)0x80000000;

#include <fstream>
#include <chrono>
#include <iostream>//TO DO swap "logger"
#include <thread>
#include <mutex>



/**
 * @brief Класс реализует паттерн наблюдатель. Работает в многопоточном режиме. Состояние наблюдателя обновляется
 *          в наблюдаемом потоке. А в потоке работы наблюдателя, логируются зависания состояния наблюдателя, как
 *          следствие замирания наблюдаемого потока.
 *          В наблюдаемом потоке состояние наблюдателя задается вркчную (номера контрольных точек) с помощью метода
 *          Update, в дальнейшем, при анализе логг файлов, можно увидеть в каком конктретном месте замирает поток.
 *          Для случая краха всего процесса, сохраняется значение состояние в файл (предполагается каталог tmp) для
 *          возможности диагности момента краха. После перезапкуска процесса (предполагается скриптом), проверяется
 *          данный файл, и логгируется момент последнего состояния.
 */
class observer_t
{
protected:
	/**
	 * @brief Метод сохранения состояния наблюдателя, для случая краха всего процесса.
	 *
	 * @param s32_state -- порядковый номер состояния.
	 */
	void SaveLastState(int s32_state);

	/**
	 * @brief Метод проверки состояния с момента прошлой рабочей сессии.
	 *
	 * @return -- Прошлое сохраненное состояние, либо, если файл не был создан (штатный запуск) то STATE_INIT_VALUE
	 */
	int CheckLastState();

public:
	/**
	 * @brief Конструктор с 4-мя параметрами.
	 *
	 * @param s32_period -- период между наблюдениями, мс
	 * @param sz_lastStateFile -- путь и название файла для прошлого значения состояния (каталог tmp)
	 * @param sz_observeName -- имя объекта наблюдения
	 * @param r_logger -- ссылка на логгер  
	 */
	observer_t(int s32_period, const char* sz_lastStateFile, const char* sz_observeName, log_t& r_logger);

	/**
	 * @brief Деструктор.
	 */
	~observer_t();

	/**
	 * @brief Метод обновления состояния наблюдателя, вызывается в наблюдаемом потоке.
	 *
	 * @param s32_state -- порядковый номер состояния.
	 */
	void Update(int s32_state);

	/**
	 * @brief Метод работы наблюдателя (логгирование замираний), запускается в потоке работы наблюдателя.
	 */
	void Work();

private:
	volatile bool m_b_threadStop; //флаг остановки потока наблюдателя
	volatile int m_s32_state; //состояние наблюдателя
	int m_s32_oldHeartBeat; //старый hb
	volatile int m_s32_curHeartBeat; //новый hb
	int m_s32_period; // период между наблюдениями
	std::thread m_h_observerMain; //Дескриптор потока наблюдателя
	std::mutex m_h_observerMtx;//Мьютекс для защиты данных.
	std::string m_sz_observeName; // название объекта наблюдения
	log_t& m_r_logger; // Лог файл
	std::fstream m_h_fileLastState; // файл хранения прошлого состояния 
};

////////////cpp

/**
* @brief Конструктор с 3-мя параметрами.
*
* @param sz_lastStateFile -- путь и название файла записи последнего состояния (запись ведется каждый вызов Update, пердпологается использовать файл в ОЗУ)
* @param sz_observeName -- имя объекта наблюдения
* @param r_logger -- ссылка на логгер
*/
observer_t::observer_t(int s32_period, const char* sz_lastStateFile, const char* sz_observeName, log_t& r_logger) :
	m_b_threadStop(false), m_s32_state(STATE_INIT_VALUE), m_s32_oldHeartBeat(0), m_s32_curHeartBeat(0), 
	m_s32_period(s32_period), m_h_observerMain([this]() { Work(); }), m_sz_observeName(sz_observeName), m_r_logger(r_logger)
{
	// Проверяем файл прошлого состояния
	m_h_fileLastState.open(sz_lastStateFile);

	int s32_tempState = CheckLastState();
	if (s32_tempState != STATE_INIT_VALUE)
		m_r_logger.doLog( m_sz_observeName + " last frize in state : " + std::to_string(s32_tempState) + '\n');
}

/**
* @brief Деструктор.
*/
observer_t::~observer_t()
{
	// останов и ожидание потока, удаление мьютекса
	m_h_observerMtx.lock();
	m_b_threadStop = true;
	m_h_observerMtx.unlock();
	m_h_observerMain.join();

	SaveLastState(STATE_INIT_VALUE);
}

/**
 * @brief Метод сохранения состояния наблюдателя, для случая краха всего процесса.
 *
 * @param s32_state -- порядковый номер состояния.
 */
void observer_t::SaveLastState(int s32_state)
{
	if (m_h_fileLastState)
	{
		m_h_fileLastState.clear(); // если eof нужен сброс
		m_h_fileLastState.seekg(std::ios::beg); // устанавливаем указатель в начало
		m_h_fileLastState << s32_state; 
		m_h_fileLastState.flush(); // запись
	}
}

/**
 * @brief Метод проверки состояния с момента прошлой рабочей сессии.
 *
 * @return -- Прошлое сохраненное состояние, либо, если файл не был создан (штатный запуск) то STATE_INIT_VALUE
 */
int observer_t::CheckLastState()
{
	int s32_result = STATE_INIT_VALUE;

	if (m_h_fileLastState)
	{
		m_h_fileLastState.clear();
		m_h_fileLastState.seekg(std::ios::beg);
		if (!m_h_fileLastState.eof())
			m_h_fileLastState >> s32_result;
	}
	return s32_result;
}

/**
* @brief Метод обновления состояния наблюдателя, вызывается в наблюдаемом потоке.
*
* @param s32_state -- порядковый номер состояния.
*/
void observer_t::Update(int s32_state)
{// обновление состояния наблюдаемой функции. вызывать с наблюдаемой функции
	m_h_observerMtx.lock();
	m_s32_state = s32_state;
	++m_s32_curHeartBeat;
	m_h_observerMtx.unlock();

	SaveLastState(s32_state);
}

/**
* @brief Метод работы наблюдателя (логгирование замираний), запускается в потоке работы наблюдателя.
*/
void observer_t::Work()
{// основной метод работы наблюдателя
	m_r_logger.doLog(m_sz_observeName + "_observer started\n");

	int s32_tempHeartBeat = 0;
	int s32_tempState = 0;
	bool b_stop = false;

	while (!b_stop)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(m_s32_period));
			//обновление разделяемых данных
		m_h_observerMtx.lock();
		s32_tempHeartBeat = m_s32_curHeartBeat;
		s32_tempState = m_s32_state;
		b_stop = m_b_threadStop;
		m_h_observerMtx.unlock();

		if (!b_stop)
		{
			if (m_s32_oldHeartBeat != s32_tempHeartBeat || s32_tempState == STATE_INIT_VALUE)
				m_s32_oldHeartBeat = s32_tempHeartBeat; // активность есть или update не вызывался ни разу - гуд, работаем дальше
			else
			{// если нет, пишем лог и останавливаемся
				m_r_logger.doLog(m_sz_observeName + " frize in state : " + std::to_string(s32_tempState) + '\n');
				b_stop = true;
			}
		}
	}//while
	m_r_logger.doLog(m_sz_observeName + "_observer stoped\n");
}

