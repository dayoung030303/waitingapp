#include <iostream>
#include <ncurses.h>
#include <signal.h>
#include <string>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAX_WAITING_NUMBER 5

using namespace std;

// 대기 중인 사람을 저장하는 노드
struct Node {
    string name;
    int waitingNumber;
    time_t startTime; // 대기 시작 시간을 저장하기 위해 추가
    struct Node *next;
};

// 대기열을 저장하는 큐
struct Queue {
    struct Node *front, *rear;
    int currentWaitingNumber;
};

// 큐 초기화 함수
void initQueue(struct Queue *queue) {
    queue->front = queue->rear = nullptr;
    queue->currentWaitingNumber = 1;
}

// 새로운 사람을 큐에 추가하는 함수
void enqueue(struct Queue *queue, const string &name) {

    if (queue->currentWaitingNumber > MAX_WAITING_NUMBER) {
        attron(COLOR_PAIR(1));
        mvprintw(5, 0, "error: waiting end");
        attroff(COLOR_PAIR(1));
        refresh();
        sleep(2);
        mvprintw(5, 0, "                          ");
        refresh();
        return;
    }

    struct Node *newNode = new Node;
    newNode->name = name;
    newNode->waitingNumber = queue->currentWaitingNumber++;
    newNode->startTime = time(nullptr); // 사람이 대기를 시작한 시간을 기록
    newNode->next = nullptr;

    // 화면 전체를 지우기
    clear();
    refresh();

    // 대기 번호를 표시하는 새 창 생성
    WINDOW *waitingWin = newwin(5, 20, LINES / 2 - 2, COLS / 2 - 10);
    box(waitingWin, 0, 0);
    mvwprintw(waitingWin, 2, 6, "Waiting %d", newNode->waitingNumber);
    wrefresh(waitingWin);

    // 대기 번호를 화면에 5초 동안 표시
    sleep(5);

    // 대기 중인 창 닫기
    werase(waitingWin);
    wrefresh(waitingWin);
    delwin(waitingWin);

    // 사람을 큐에 추가
    if (newNode->waitingNumber <= MAX_WAITING_NUMBER) {
        if (queue->rear == nullptr) {
            queue->front = queue->rear = newNode;
        } else {
            queue->rear->next = newNode;
            queue->rear = newNode;
        }
    }

    return;
}

// 큐에서 사람을 제거하고 그 이름을 반환하는 함수
string dequeue(struct Queue *queue) {
    if (queue->front == nullptr) {
        return ""; // 큐가 비어 있으면 빈 문자열 반환
    }

    struct Node *temp = queue->front;
    string name = temp->name;

    queue->front = temp->next;
    delete temp;

    if (queue->front == nullptr) {
        queue->rear = nullptr;
    }

    return name;
}

// 큐의 크기를 가져오는 함수
int getQueueSize(struct Queue *queue) {
    int size = 0;
    struct Node *current = queue->front;
    while (current != nullptr) {
        size++;
        current = current->next;
    }
    return size;
}

// 사람이 대기를 시작한 후 40 지났는지 확인하는 함수
void checkElapsedTime(struct Queue *queue) {
    time_t currentTime = time(nullptr);

    if (queue->front != nullptr &&
        (currentTime - queue->front->startTime) >= 40) {
        // 40이 지났으면 사람을 큐에서 제거
        string name = dequeue(queue);
        struct Node *current = queue->front;

        mvprintw(7, 0, "Waiting number %d, Team '%s' enter.",
                 current->waitingNumber - 1, name.c_str());
        refresh();
        sleep(5);
        mvprintw(7, 0, "                                             ");
        refresh();

        if (getQueueSize(queue) == 1) {
            mvprintw(10, 0,
                     "Last person in the waiting line. Program will exit.");
            refresh();
            sleep(5);
            endwin(); // ncurses 종료
            exit(0);
        }

        return;
    }
}

// 큐에 있는 사람들을 화면에 표시하는 함수
void displayQueue(struct Queue *queue) {
    attron(COLOR_PAIR(3));
    mvprintw(0, 0, "Enter the name:");
    mvprintw(2, 0, "Total waiting team: %d", getQueueSize(queue));
    /*
        int row = 11;
        Node *current = queue->front;
        while (current != nullptr) {
            attron(A_UNDERLINE);
            mvprintw(row, 0, "%s (Waiting Number: %d)", current->name.c_str(),
                     current->waitingNumber);
            attroff(A_UNDERLINE);
            row++;
            current = current->next;
        }
    */
    attroff(COLOR_PAIR(3));
}

// 시그널 핸들러 함수
void alrmsignalHandler(int signum) { mvprintw(10, 0, "Start enter!"); }

int main() {
    // 초기화
    initscr();     // ncurses 초기화
    start_color(); // 색상 활성화

    // 색상 페어 정의
    init_pair(1, COLOR_RED, COLOR_BLACK);   // 에러 메시지 색상
    init_pair(2, COLOR_GREEN, COLOR_BLACK); // 성공 메시지 색상
    init_pair(3, COLOR_CYAN, COLOR_BLACK);  // 큐 표시 색상
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);

    clear();
    refresh();

    keypad(stdscr, TRUE); // 특수 키 입력 활성화
    noecho();             // 입력된 문자 표시하지 않음

    // 시그널 핸들러 등록
    if (signal(SIGALRM, alrmsignalHandler) == SIG_ERR) {
        perror("signal() error!");
    }

    alarm(30);

    // 큐 초기화
    struct Queue waitingQueue;
    initQueue(&waitingQueue);

    // 사람의 이름을 입력하고 큐를 표시하는 화면을 생성
    mvprintw(0, 0, "Enter the name:");

    refresh();

    string name = "aaaaaaaaaa";

    // 메인 루프
    while (1) {
        checkElapsedTime(
            &waitingQueue); // 큐 맨 앞의 사람이 1분이 지났는지 확인
        mvgetnstr(0, 15, &name[0],
                  name.capacity() - 1); // 사용자로부터 이름을 얻음
        int ch = getch();               // 눌린 키를 얻음

        // 눌린 키가 스페이스바인지 확인
        if (ch == ' ') {
            mvprintw(10, 0, "Exiting program...");
            refresh();
            sleep(5);
            endwin(); // ncurses 종료
            exit(0);
        }

        while (name.length() > 10) {
            attron(COLOR_PAIR(3)); // 색상 속성 설정
            mvprintw(5, 0, "error: excess size");
            attroff(COLOR_PAIR(3)); // 색상 속성 설정
            refresh();
            sleep(2); // 에러 메시지를 2초 동안 표시
            mvprintw(
                5, 0,
                "                                      "); // 에러 메시지 지우기
            refresh();
            mvprintw(0, 15, "                    "); // 입력 영역 지우기
            refresh();
            mvgetnstr(0, 15, &name[0],
                      name.capacity() - 1); // 사용자로부터 이름 다시 얻기
            refresh();
        }

        enqueue(&waitingQueue,
                name); // 유효성 검사를 수행하여 사람을 큐에 추가
        attron(COLOR_PAIR(1));
        displayQueue(&waitingQueue); // 화면에 큐 표시
        attroff(COLOR_PAIR(1));
        refresh();

        mvprintw(0, 25, "                   "); // 입력 영역 지우기
        refresh();
    }

    // 종료
    endwin(); // ncurses 종료

    return 0;
}
