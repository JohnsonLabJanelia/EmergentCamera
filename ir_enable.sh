echo 'nvidia' | sudo -S chmod a+wrx /sys/kernel/debug
sudo chmod a+wrx /sys/kernel/debug/pca9570_a/pca9570
echo n > /sys/kernel/debug/pca9570_a/pca9570
exit
