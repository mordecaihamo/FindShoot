using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace ShotTracker
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
#if DEBUG
        [DllImport("FindShootd.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int FindShoots(string vidName);
#else
        [DllImport("FindShoot.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int FindShoots(string vidName);
#endif

        public MainWindow()
        {
            InitializeComponent();
        }

        private void BtnPlay_Click(object sender, RoutedEventArgs e)
        {
            string vidName = "C:\\moti\\FindShoot\\MVI_3.MOV";
            int res = FindShoots(vidName);
        }
    }
}
