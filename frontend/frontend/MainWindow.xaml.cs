using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Threading;

namespace frontend
{
    public partial class MainWindow : Window
    {
        private readonly MotorController xMotor = new("27000001");
        private readonly MotorController yMotor = new("27000002");
        private readonly DispatcherTimer positionTimer;

        private bool xConnected;
        private bool yConnected;
        private bool xDriving;
        private bool yDriving;
        private int activeMovementCommands;

        public MainWindow()
        {
            InitializeComponent();

            positionTimer = new DispatcherTimer
            {
                Interval = TimeSpan.FromSeconds(1)
            };

            positionTimer.Tick += UpdatePositions;
            positionTimer.Start();
        }

        private void MainWindow_Closing(object? sender, CancelEventArgs e)
        {
            if (activeMovementCommands > 0)
            {
                e.Cancel = true;
                MessageBox.Show("Wait for the active motor command to finish before closing.");
            }
        }

        private void MainWindow_Closed(object? sender, EventArgs e)
        {
            positionTimer.Stop();

            if (xConnected)
            {
                xMotor.Close();
            }

            if (yConnected)
            {
                yMotor.Close();
            }
        }

        private void MainWindow_Deactivated(object? sender, EventArgs e)
        {
            StopXDrive();
            StopYDrive();
        }

        private void UpdatePositions(object? sender, EventArgs e)
        {
            if (xConnected)
            {
                UpdatePosition(xMotor, XCurrentPositionText);
            }

            if (yConnected)
            {
                UpdatePosition(yMotor, YCurrentPositionText);
            }
        }

        private static void UpdatePosition(MotorController motor, TextBlock positionText)
        {
            double position = motor.GetPosition();
            positionText.Text = $"Current position: {position:F3} mm";
        }

        private void XConnect_Click(object sender, RoutedEventArgs e)
        {
            int rc = xMotor.Connect();

            if (rc == 0)
            {
                xConnected = true;
                XStatusText.Text = "Status: Connected";
                XConnectButton.IsEnabled = false;
                SetXMovementEnabled(true);
                UpdatePosition(xMotor, XCurrentPositionText);
            }
            else
            {
                XStatusText.Text = $"Status: Connection failed ({rc})";
            }
        }

        private void YConnect_Click(object sender, RoutedEventArgs e)
        {
            int rc = yMotor.Connect();

            if (rc == 0)
            {
                yConnected = true;
                YStatusText.Text = "Status: Connected";
                YConnectButton.IsEnabled = false;
                SetYMovementEnabled(true);
                UpdatePosition(yMotor, YCurrentPositionText);
            }
            else
            {
                YStatusText.Text = $"Status: Connection failed ({rc})";
            }
        }

        private async void XMoveAbsolute_Click(object sender, RoutedEventArgs e)
        {
            if (!double.TryParse(XPositionBox.Text, out double position))
            {
                XStatusText.Text = "Status: Invalid position";
                return;
            }

            await ExecuteMovement(() => xMotor.MoveAbsolute(position), xMotor, XStatusText, XCurrentPositionText, SetXMovementEnabled, "Moving...");
        }

        private async void YMoveAbsolute_Click(object sender, RoutedEventArgs e)
        {
            if (!double.TryParse(YPositionBox.Text, out double position))
            {
                YStatusText.Text = "Status: Invalid position";
                return;
            }

            await ExecuteMovement(() => yMotor.MoveAbsolute(position), yMotor, YStatusText, YCurrentPositionText, SetYMovementEnabled, "Moving...");
        }

        private async void XJogForward_Click(object sender, RoutedEventArgs e) =>
            await Jog(xMotor, 1, XJogDisplacementBox, XStatusText, XCurrentPositionText, SetXMovementEnabled);

        private async void XJogBackward_Click(object sender, RoutedEventArgs e) =>
            await Jog(xMotor, -1, XJogDisplacementBox, XStatusText, XCurrentPositionText, SetXMovementEnabled);

        private async void YJogForward_Click(object sender, RoutedEventArgs e) =>
            await Jog(yMotor, 1, YJogDisplacementBox, YStatusText, YCurrentPositionText, SetYMovementEnabled);

        private async void YJogBackward_Click(object sender, RoutedEventArgs e) =>
            await Jog(yMotor, -1, YJogDisplacementBox, YStatusText, YCurrentPositionText, SetYMovementEnabled);

        private async Task Jog(
            MotorController motor,
            int direction,
            TextBox displacementBox,
            TextBlock statusText,
            TextBlock positionText,
            Action<bool> setMovementEnabled)
        {
            if (!double.TryParse(displacementBox.Text, out double displacement) || displacement <= 0)
            {
                statusText.Text = "Status: Invalid jog displacement";
                return;
            }

            await ExecuteMovement(() => motor.Jog(direction, displacement), motor, statusText, positionText, setMovementEnabled, "Jogging...");
        }

        private async void XHome_Click(object sender, RoutedEventArgs e) =>
            await ExecuteMovement(xMotor.Home, xMotor, XStatusText, XCurrentPositionText, SetXMovementEnabled, "Homing...");

        private async void YHome_Click(object sender, RoutedEventArgs e) =>
            await ExecuteMovement(yMotor.Home, yMotor, YStatusText, YCurrentPositionText, SetYMovementEnabled, "Homing...");

        private async Task ExecuteMovement(Func<int> command, MotorController motor, TextBlock statusText, TextBlock positionText, Action<bool> setMovementEnabled, string activity)
        {
            setMovementEnabled(false);
            statusText.Text = $"Status: {activity}";
            activeMovementCommands++;

            int rc;
            try
            {
                rc = await Task.Run(command);
            }
            finally
            {
                activeMovementCommands--;
                setMovementEnabled(true);
            }

            if (rc == 0)
            {
                statusText.Text = "Status: Ready";
                UpdatePosition(motor, positionText);
            }
            else
            {
                statusText.Text = $"Status: Motor command failed ({rc})";
            }
        }

        private void XDriveBackward_Down(object sender, MouseButtonEventArgs e) =>
            StartDrive(xMotor, -1, ref xDriving, XStatusText);

        private void XDriveForward_Down(object sender, MouseButtonEventArgs e) =>
            StartDrive(xMotor, 1, ref xDriving, XStatusText);

        private void YDriveBackward_Down(object sender, MouseButtonEventArgs e) =>
            StartDrive(yMotor, -1, ref yDriving, YStatusText);

        private void YDriveForward_Down(object sender, MouseButtonEventArgs e) =>
            StartDrive(yMotor, 1, ref yDriving, YStatusText);

        private void XDrive_MouseUp(object sender, MouseButtonEventArgs e) => StopXDrive();

        private void YDrive_MouseUp(object sender, MouseButtonEventArgs e) => StopYDrive();

        private void XDrive_LostMouseCapture(object sender, MouseEventArgs e) => StopXDrive();

        private void YDrive_LostMouseCapture(object sender, MouseEventArgs e) => StopYDrive();

        private static void StartDrive(
            MotorController motor,
            int direction,
            ref bool driving,
            TextBlock statusText)
        {
            if (driving)
            {
                return;
            }

            int rc = motor.StartDrive(direction);

            if (rc == 0)
            {
                driving = true;
                statusText.Text = "Status: Driving...";
            }
            else
            {
                statusText.Text = $"Status: Drive failed ({rc})";
            }
        }

        private void StopXDrive() =>
            StopDrive(xMotor, ref xDriving, XStatusText, XCurrentPositionText);

        private void StopYDrive() =>
            StopDrive(yMotor, ref yDriving, YStatusText, YCurrentPositionText);

        private static void StopDrive(
            MotorController motor,
            ref bool driving,
            TextBlock statusText,
            TextBlock positionText)
        {
            if (!driving)
            {
                return;
            }

            driving = false;
            int rc = motor.StopDrive();

            if (rc == 0)
            {
                statusText.Text = "Status: Ready";
                UpdatePosition(motor, positionText);
            }
            else
            {
                statusText.Text = $"Status: Stop failed ({rc})";
            }
        }

        private void SetXMovementEnabled(bool enabled)
        {
            XPositionBox.IsEnabled = enabled;
            XJogDisplacementBox.IsEnabled = enabled;
            XMoveButton.IsEnabled = enabled;
            XJogBackwardButton.IsEnabled = enabled;
            XJogForwardButton.IsEnabled = enabled;
            XDriveBackwardButton.IsEnabled = enabled;
            XDriveForwardButton.IsEnabled = enabled;
            XHomeButton.IsEnabled = enabled;
        }

        private void SetYMovementEnabled(bool enabled)
        {
            YPositionBox.IsEnabled = enabled;
            YJogDisplacementBox.IsEnabled = enabled;
            YMoveButton.IsEnabled = enabled;
            YJogBackwardButton.IsEnabled = enabled;
            YJogForwardButton.IsEnabled = enabled;
            YDriveBackwardButton.IsEnabled = enabled;
            YDriveForwardButton.IsEnabled = enabled;
            YHomeButton.IsEnabled = enabled;
        }
    }
}
