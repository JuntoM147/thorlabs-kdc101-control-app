using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Threading;

namespace frontend
{
    public partial class MainWindow : Window
    {
        private sealed class AxisState(MotorController motor, TextBlock statusText, Button connectButton, TextBox positionBox, TextBox jogDisplacementBox, TextBlock currentPositionText, params Control[] movementControls)
        {
            public MotorController Motor { get; } = motor;
            public TextBlock StatusText { get; } = statusText;
            public Button ConnectButton { get; } = connectButton;
            public TextBox PositionBox { get; } = positionBox;
            public TextBox JogDisplacementBox { get; } = jogDisplacementBox;
            public TextBlock CurrentPositionText { get; } = currentPositionText;
            public Control[] MovementControls { get; } = movementControls;
            public bool Connected { get; set; }
            public bool Driving { get; set; }

            public void SetMovementEnabled(bool enabled)
            {
                PositionBox.IsEnabled = enabled;
                JogDisplacementBox.IsEnabled = enabled;

                foreach (Control control in MovementControls)
                {
                    control.IsEnabled = enabled;
                }
            }
        }

        private readonly Dictionary<string, AxisState> axes;
        private readonly DispatcherTimer positionTimer;
        private int activeMovementCommands;

        public MainWindow()
        {
            InitializeComponent();

            axes = new Dictionary<string, AxisState>
            {
                ["X"] = CreateAxis("27000001", XStatusText, XConnectButton, XPositionBox, XJogDisplacementBox, XCurrentPositionText, XMoveButton, XJogBackwardButton, XJogForwardButton, XDriveBackwardButton, XDriveForwardButton, XHomeButton),
                ["Y"] = CreateAxis("27000002", YStatusText, YConnectButton, YPositionBox, YJogDisplacementBox, YCurrentPositionText, YMoveButton, YJogBackwardButton, YJogForwardButton, YDriveBackwardButton, YDriveForwardButton, YHomeButton),
                ["Z"] = CreateAxis("27000003", ZStatusText, ZConnectButton, ZPositionBox, ZJogDisplacementBox, ZCurrentPositionText, ZMoveButton, ZJogBackwardButton, ZJogForwardButton, ZDriveBackwardButton, ZDriveForwardButton, ZHomeButton)
            };

            positionTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(1) };
            positionTimer.Tick += UpdatePositions;
            positionTimer.Start();
        }

        private static AxisState CreateAxis(string serialNumber, TextBlock statusText, Button connectButton, TextBox positionBox, TextBox jogDisplacementBox, TextBlock currentPositionText, params Control[] movementControls) => new(new MotorController(serialNumber), statusText, connectButton, positionBox, jogDisplacementBox, currentPositionText, movementControls);

        private AxisState GetAxis(object sender)
        {
            string axisName = (string)((FrameworkElement)sender).Tag;
            return axes[axisName];
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
            foreach (AxisState axis in axes.Values.Where(axis => axis.Connected))
            {
                axis.Motor.Close();
            }
        }

        private void MainWindow_Deactivated(object? sender, EventArgs e)
        {
            foreach (AxisState axis in axes.Values)
            {
                StopDrive(axis);
            }
        }

        private void UpdatePositions(object? sender, EventArgs e)
        {
            foreach (AxisState axis in axes.Values.Where(axis => axis.Connected))
            {
                UpdatePosition(axis);
            }
        }

        private static void UpdatePosition(AxisState axis)
        {
            double position = axis.Motor.GetPosition();
            axis.CurrentPositionText.Text = $"Current position: {position:F3} mm";
        }

        private void Connect_Click(object sender, RoutedEventArgs e)
        {
            AxisState axis = GetAxis(sender);
            int rc = axis.Motor.Connect();

            if (rc == 0)
            {
                axis.Connected = true;
                axis.StatusText.Text = "Status: Connected";
                axis.ConnectButton.IsEnabled = false;
                axis.SetMovementEnabled(true);
                UpdatePosition(axis);
            }
            else
            {
                axis.StatusText.Text = $"Status: Connection failed ({rc})";
            }
        }

        private async void MoveAbsolute_Click(object sender, RoutedEventArgs e)
        {
            AxisState axis = GetAxis(sender);
            if (!double.TryParse(axis.PositionBox.Text, out double position))
            {
                axis.StatusText.Text = "Status: Invalid position";
                return;
            }

            await ExecuteMovement(axis, () => axis.Motor.MoveAbsolute(position), "Moving...");
        }

        private async void JogForward_Click(object sender, RoutedEventArgs e) =>
            await Jog(GetAxis(sender), 1);

        private async void JogBackward_Click(object sender, RoutedEventArgs e) =>
            await Jog(GetAxis(sender), -1);

        private async Task Jog(AxisState axis, int direction)
        {
            if (!double.TryParse(axis.JogDisplacementBox.Text, out double displacement) || displacement <= 0)
            {
                axis.StatusText.Text = "Status: Invalid jog displacement";
                return;
            }

            await ExecuteMovement(axis, () => axis.Motor.Jog(direction, displacement), "Jogging...");
        }

        private async void Home_Click(object sender, RoutedEventArgs e)
        {
            AxisState axis = GetAxis(sender);
            await ExecuteMovement(axis, axis.Motor.Home, "Homing...");
        }

        private async Task ExecuteMovement(AxisState axis, Func<int> command, string activity)
        {
            axis.SetMovementEnabled(false);
            axis.StatusText.Text = $"Status: {activity}";
            activeMovementCommands++;

            int rc;
            try
            {
                rc = await Task.Run(command);
            }
            finally
            {
                activeMovementCommands--;
                axis.SetMovementEnabled(true);
            }

            if (rc == 0)
            {
                axis.StatusText.Text = "Status: Ready";
                UpdatePosition(axis);
            }
            else
            {
                axis.StatusText.Text = $"Status: Motor command failed ({rc})";
            }
        }

        private void DriveBackward_Down(object sender, MouseButtonEventArgs e) =>
            StartDrive(GetAxis(sender), -1);

        private void DriveForward_Down(object sender, MouseButtonEventArgs e) =>
            StartDrive(GetAxis(sender), 1);

        private void Drive_MouseUp(object sender, MouseButtonEventArgs e) => StopDrive(GetAxis(sender));

        private void Drive_LostMouseCapture(object sender, MouseEventArgs e) => StopDrive(GetAxis(sender));

        private static void StartDrive(AxisState axis, int direction)
        {
            if (axis.Driving)
            {
                return;
            }

            int rc = axis.Motor.StartDrive(direction);
            if (rc == 0)
            {
                axis.Driving = true;
                axis.StatusText.Text = "Status: Driving...";
            }
            else
            {
                axis.StatusText.Text = $"Status: Drive failed ({rc})";
            }
        }

        private static void StopDrive(AxisState axis)
        {
            if (!axis.Driving)
            {
                return;
            }

            axis.Driving = false;
            int rc = axis.Motor.StopDrive();
            if (rc == 0)
            {
                axis.StatusText.Text = "Status: Ready";
                UpdatePosition(axis);
            }
            else
            {
                axis.StatusText.Text = $"Status: Stop failed ({rc})";
            }
        }
    }
}
