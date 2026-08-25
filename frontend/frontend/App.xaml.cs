using System.Windows;

namespace frontend
{
    public partial class App : Application
    {
        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);

            MotorController.InitializeSimulation();

            MainWindow = new MainWindow();
            MainWindow.Show();
        }

        protected override void OnExit(ExitEventArgs e)
        {
            MotorController.UninitializeSimulation();

            base.OnExit(e);
        }
    }
}
