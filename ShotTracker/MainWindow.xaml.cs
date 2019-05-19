using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
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
        //extern "C" FINDSHOOTEXPORT int Analyze(char* vidName, int isDebugMode);
        [DllImport("FindShootd.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int FindShoots(string vidName, IntPtr imgBuffer, int imgWidth, int imgHeight, int isDebugMode);

        [DllImport("FindShootd.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int Analyze(string vidName, int isDebugMode);
#else
        [DllImport("FindShoot.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int FindShoots(string vidName, IntPtr imgBuf, int width, int height, int isDbgMode = 1);

        [DllImport("FindShoot.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int Analyze(string vidName, int isDebugMode);
#endif
        string mVidFile;

        public MainWindow()
        {
            InitializeComponent();
        }

        private void BtnPlay_Click(object sender, RoutedEventArgs e)
        {
            mVidFile = "C:/moti/FindShoot/videos/MVI_3.MOV";
            if (mVidFile == null)
                return;
            int bmWidth = 800;
            int bmHeight = bmWidth * 4 / 3;
            int isDebugMode = chkBoxIsDbg.IsChecked == false ? 0 : 1;
            
            Bitmap managedBitmap = new Bitmap(bmWidth, bmHeight, PixelFormat.Format24bppRgb);
            //BitmapData bmpData = managedBitmap.LockBits(new Rectangle(0, 0, managedBitmap.Width, managedBitmap.Height), System.Drawing.Imaging.ImageLockMode.ReadWrite,
            //            System.Drawing.Imaging.PixelFormat.Format24bppRgb);
            
            Task task1 = Task.Factory.StartNew(() => FindShoots(mVidFile, managedBitmap.GetHbitmap(), managedBitmap.Width, managedBitmap.Height, isDebugMode));
            //while (!task1.IsCompleted)
            //{             //int res = FindShoots(mVidFile, bmpData.Scan0, managedBitmap.Width, managedBitmap.Height);
            //    Thread.Sleep(40);
            //   // Graphics g = Graphics.FromImage(managedBitmap);
                
                
            //}
            //task1.Wait();
            //managedBitmap.UnlockBits(bmpData); //Remember to unlock!!!            
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

        private void BtnAnalyze_Click(object sender, RoutedEventArgs e)
        {
            mVidFile = "C:/moti/FindShoot/videos/MVI_3.MOV";
            if (mVidFile == null)
                return;
            int bmWidth = 800;
            int bmHeight = bmWidth * 4 / 3;
            int isDebugMode = chkBoxIsDbg.IsChecked == false ? 0 : 1;
            Task task1 = Task.Factory.StartNew(() => Analyze(mVidFile, isDebugMode));
            task1.Wait();
        }
    }
}
