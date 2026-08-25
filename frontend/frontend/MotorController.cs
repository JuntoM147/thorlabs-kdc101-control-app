using System.Runtime.InteropServices;

namespace frontend
{
    public class MotorController
    {
        private readonly string serialNo;

        [DllImport("kdc101.dll")]
        private static extern void initialize_simulation();

        [DllImport("kdc101.dll")]
        private static extern void uninitialize_simulation();

        [DllImport("kdc101.dll")]
        private static extern int connect_device(string serialNo);

        [DllImport("kdc101.dll")]
        private static extern int home_device(string serialNo);

        [DllImport("kdc101.dll")]
        private static extern int move_position(string serialNo, double position);

        [DllImport("kdc101.dll")]
        private static extern double get_position(string serialNo);

        [DllImport("kdc101.dll")]
        private static extern int jog(string serialNo, int direction, double displacement);

        [DllImport("kdc101.dll")]
        private static extern int start_drive(string serialNo, int direction);

        [DllImport("kdc101.dll")]
        private static extern int stop_drive(string serialNo);

        [DllImport("kdc101.dll")]
        private static extern int close_device(string serialNo);

        public MotorController(string serialNo)
        {
            this.serialNo = serialNo;
        }

        public static void InitializeSimulation()
        {
            initialize_simulation();
        }

        public static void UninitializeSimulation()
        {
            uninitialize_simulation();
        }

        public int Connect()
        {
            return connect_device(serialNo);
        }

        public int Home()
        {
            return home_device(serialNo);
        }

        public int MoveAbsolute(double position)
        {
            return move_position(serialNo, position);
        }

        public double GetPosition()
        {
            return get_position(serialNo);
        }

        public int Jog(int direction, double displacement)
        {
            return jog(serialNo, direction, displacement);
        }

        public int StartDrive(int direction)
        {
            return start_drive(serialNo, direction);
        }

        public int StopDrive()
        {
            return stop_drive(serialNo);
        }

        public int Close()
        {
            return close_device(serialNo);
        }
    }
}
