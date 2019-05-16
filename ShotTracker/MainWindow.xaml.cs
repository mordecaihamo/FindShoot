using System.Drawing;
using System.Runtime.InteropServices;
using System.Windows;
using PixelFormat = System.Drawing.Imaging.PixelFormat;

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
        string mVidFile;

        public MainWindow()
        {
            InitializeComponent();
        }

        private void BtnPlay_Click(object sender, RoutedEventArgs e)
        {
            if (mVidFile == null)
                return;
            int bmWidth = 800;
            int bmHeight = bmWidth * 4 / 3;
            Bitmap managedBitmap = new Bitmap(bmWidth, bmHeight, PixelFormat.Format24bppRgb);


            int res = FindShoots(mVidFile);

        }

        private void BtnBrowseVidFile_Click(object sender, RoutedEventArgs e)
        {
            var fileDialog = new System.Windows.Forms.OpenFileDialog();
            var result = fileDialog.ShowDialog();
            switch (result)
            {
                case System.Windows.Forms.DialogResult.OK:
                    var file = fileDialog.FileName;
                    txtVidFile.Text = file;
                    txtVidFile.ToolTip = file;
                    break;
                case System.Windows.Forms.DialogResult.Cancel:
                default:
                    txtVidFile.Text = null;
                    txtVidFile.ToolTip = null;
                    break;
            }
        }
    }
}
