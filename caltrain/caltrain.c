#include "pintos_thread.h"

//cấu trúc station
struct station {
    int free_seats; //số ghế ngồi còn trống
    int passengers_waiting; // số khách hàng đang chờ ở ga
    int passengers_leaving; // số khách hàng di chuyển khi đã tìm thấy ghế trống
    //biến điều kiện
    pthread_mutex_t mutex; //khai báo mutex (mutex là con trỏ đến biến cấu trúc pthread_mutex_t)
    pthread_cond_t train_arrived; // tàu đến với chỗ ngồi ngồi trống
    pthread_cond_t passengers_seated; //hành khách đã ngồi xuống
};

//hàm khởi tạo 
void station_init(struct station *station) {
    //khởi tạo biến
    station->free_seats = 0;
    station->passengers_waiting = 0;
    station->passengers_leaving = 0;
    //khởi tạo mutex và biến điều kiện
    pthread_mutex_init(&(station->mutex), NULL);
    pthread_cond_init(&(station->train_arrived), NULL);
    pthread_cond_init(&(station->passengers_seated), NULL);
}

//tàu trong ga
//khi tàu đến ga và mở cửa sẽ gọi 
//count cho biết bao nhiêu chỗ ngồi trên tàu
/*chức năng không được hoạt động trở lại cho đến khi tàu được xếp đủ chỗ
(tất cả hành khách đã ngồi vào ghế, tàu đã đầy hoặc khi các hành khách đang chờ lên tàu)
*/
void station_load_train(struct station *station, int count) {
     //khóa mutex, dùng tài nguyên cho hàm
    pthread_mutex_lock(&(station->mutex));
    //!count: không có chỗ ngồi trống 
    //!station->passengers_waiting: không có hành khách đang chờ
    if (!count || !station->passengers_waiting) { 
        //mở khóa mutex
        pthread_mutex_unlock(&(station->mutex)); 
        return; 
    }
    //có chỗ ngồi trống => gán station->free_seats = count
    station->free_seats = count;
    pthread_cond_broadcast(&(station->train_arrived)); 
    pthread_cond_wait(&(station->passengers_seated), &(station->mutex)); // waiting for all passengers to get on board
    station->free_seats = 0;
    pthread_mutex_unlock(&(station->mutex)); 
}

//ga chờ tàu
//khi robot khách đến nhà ga sẽ gọi 
/*chức năng không được hoạt động trở lại cho đến khi có tàu trong ga
(tức là khi hàm station_load_train được gọi) và có đủ ghế trống trên tàu cho khách
khi chức năng hoạt động chở lại, robot khách sẽ khi chuyển và  chọn một chỗ
*/
void station_wait_for_train(struct station *station) {
    pthread_mutex_lock(&(station->mutex)); 
    //hành khách chờ tăng lên cho đến khi không còn chỗ trống
    station->passengers_waiting++;
    //khi hết ghế trống
    while (!station->free_seats)
        pthread_cond_wait(&(station->train_arrived), &(station->mutex)); 
    station->passengers_waiting--; //giảm số khách chờ
    station->passengers_leaving++; //tăng khách di chuyển
    station->free_seats--; //giảm số ghế trống
    pthread_mutex_unlock(&(station->mutex));
}


//hàm được gọi khi khách ngồi xuống để thông báo cho tàu biết
void station_on_board(struct station *station) {
    pthread_mutex_lock(&(station->mutex));
    //giảm khách di chuyển 
    station->passengers_leaving--;
    //hết khách di chuyển, hết khách chờ và hết chỗ trống
    //=> không còn hàng khách nào còn có thể lên tàu
    if (!station->passengers_leaving && !(station->passengers_waiting && station->free_seats)) 
        pthread_cond_signal(&(station->passengers_seated));
    pthread_mutex_unlock(&(station->mutex));
}
