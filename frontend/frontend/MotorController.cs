using System.Runtime.InteropServices;

namespace frontend
{
    public class MotorController
    {
        [DllImport("kdc101.dll")]
        private static extern int connect_device();

        [DllImport("kdc101.dll")]
        private static extern int home_device();

        [DllImport("kdc101.dll")]
        private static extern int move_position(double position);

        [DllImport("kdc101.dll")]
        private static extern double get_position();

        [DllImport("kdc101.dll")]
        private static extern int move_relative(double displacement);

        [DllImport("kdc101.dll")]
        private static extern int jog(int direction);

        [DllImport("kdc101.dll")]
        private static extern int close_device();


        public int Connect()
        {
            return connect_device();
        }


        public int Home()
        {
            return home_device();
        }


        public int MoveAbsolute(double position)
        {
            return move_position(position);
        }


        public int MoveRelative(double displacement)
        {
            return move_relative(displacement);
        }


        public double GetPosition()
        {
            return get_position();
        }


        public int Jog(int direction)
        {
            return jog(direction);
        }


        public int Close()
        {
            return close_device();
        }
    }
}