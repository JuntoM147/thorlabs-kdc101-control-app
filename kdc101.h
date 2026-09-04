namespace thorlabs {

typedef enum{

} foo;

class KDC101 {
    public:
        KDC101() = delete;

        explicit KDC101(int serial_number, int polling_rate = 200) : serial_number_{serial_number} {}

        int home();

        int jog();

        int drive();

        int moveAbsolute();

        ~KDC101() {}

    private:
        const int serial_number_;
};

} // thorlabs 

