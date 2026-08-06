using System.Windows;
using System.Windows.Threading;

namespace frontend
{
    public partial class MainWindow : Window
    {
        private readonly MotorController motor = new();
        private DispatcherTimer? positionTimer;


        public MainWindow()
        {
            InitializeComponent();
        }


        private void MainWindow_Closed(object? sender, EventArgs e)
        {
            positionTimer?.Stop();
            motor.Close();
        }


        private void UpdatePosition(object? sender, EventArgs e)
        {
            double position = motor.GetPosition();

            CurrentPositionText.Text = $"Current position: {position:F3} mm";
        }


        private void Connect_Click(object sender, RoutedEventArgs e)
        {
            int rc = motor.Connect();

            if (rc == 0)
            {
                StatusText.Text = "Status: Connected";

                SetConnectedState(true);

                positionTimer = new DispatcherTimer
                {
                    Interval = TimeSpan.FromSeconds(1)
                };

                positionTimer.Tick += UpdatePosition;
                positionTimer.Start();
            }
            else
            {
                StatusText.Text = "Connection failed";
            }
        }


        private async void MoveAbsolute_Click(object sender, RoutedEventArgs e)
        {
            if (!double.TryParse(PositionBox.Text, out double position))
            {
                StatusText.Text = "Invalid position";
                return;
            }

            await ExecuteMovement(() =>
                motor.MoveAbsolute(position),
                "Moving..."
            );
        }


        private async void MoveRelative_Click(object sender, RoutedEventArgs e)
        {
            if (!double.TryParse(DisplacementBox.Text, out double displacement))
            {
                StatusText.Text = "Invalid displacement";
                return;
            }

            await ExecuteMovement(() =>
                motor.MoveRelative(displacement),
                "Moving..."
            );
        }


        private async void JogForward_Click(object sender, RoutedEventArgs e)
        {
            await ExecuteMovement(() =>
                motor.Jog(1),
                "Jogging..."
            );
        }


        private async void JogBackward_Click(object sender, RoutedEventArgs e)
        {
            await ExecuteMovement(() =>
                motor.Jog(-1),
                "Jogging..."
            );
        }


        private async void Home_Click(object sender, RoutedEventArgs e)
        {
            await ExecuteMovement(() =>
                motor.Home(),
                "Homing..."
            );
        }


        private async Task ExecuteMovement(Func<int> movement, string status)
        {
            DisableMovementControls();

            StatusText.Text = $"Status: {status}";

            int rc = await Task.Run(() =>
                movement()
            );

            EnableMovementControls();

            UpdateMovementStatus(rc);
        }


        private void UpdateMovementStatus(int rc)
        {
            if (rc == 0)
            {
                StatusText.Text = "Status: Ready";

                double current = motor.GetPosition();

                CurrentPositionText.Text =
                    $"Current position: {current:F3} mm";
            }
            else
            {
                StatusText.Text = "Motor command failed";
            }
        }


        private void SetConnectedState(bool connected)
        {
            ConnectButton.IsEnabled = !connected;

            PositionBox.IsEnabled = connected;
            DisplacementBox.IsEnabled = connected;

            HomeButton.IsEnabled = connected;
            MoveButton.IsEnabled = connected;
            MoveRelativeButton.IsEnabled = connected;
            JogForwardButton.IsEnabled = connected;
            JogBackwardButton.IsEnabled = connected;
        }


        private void DisableMovementControls()
        {
            HomeButton.IsEnabled = false;
            MoveButton.IsEnabled = false;
            MoveRelativeButton.IsEnabled = false;
            JogForwardButton.IsEnabled = false;
            JogBackwardButton.IsEnabled = false;
        }


        private void EnableMovementControls()
        {
            HomeButton.IsEnabled = true;
            MoveButton.IsEnabled = true;
            MoveRelativeButton.IsEnabled = true;
            JogForwardButton.IsEnabled = true;
            JogBackwardButton.IsEnabled = true;
        }
    }
}
