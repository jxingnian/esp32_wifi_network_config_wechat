// index.ts
Page({
  data: {
    ssid: '',
    password: '',
    espStatus: false,
    wifiInfo: {
      ssid: '',
      connected: false
    },
    checkTimer: null as any
  },

  onLoad() {
    this.startAutoCheck()
  },

  onShow() {
    this.startAutoCheck()
  },

  onHide() {
    this.stopAutoCheck()
  },

  onUnload() {
    this.stopAutoCheck()
  },

  // 开始自动检测
  startAutoCheck() {
    // 先停止之前的定时器
    this.stopAutoCheck()
    
    // 立即检查一次
    this.refreshStatus()
    
    // 设置新的定时器，每秒检查一次
    this.data.checkTimer = setInterval(() => {
      this.refreshStatus()
    }, 500)
  },

  // 停止自动检测
  stopAutoCheck() {
    if (this.data.checkTimer) {
      clearInterval(this.data.checkTimer)
      this.data.checkTimer = null
    }
  },

  // 刷新ESP32状态
  refreshStatus() {
    wx.request({
      url: 'http://192.168.4.1:8080/get_status',
      method: 'GET',
      timeout: 3000,
      success: (res) => {
        this.setData({
          espStatus: true,
          wifiInfo: {
            ssid: res.data.ssid || '',
            connected: res.data.connected || false
          }
        })
      },
      fail: () => {
        this.setData({
          espStatus: false,
          wifiInfo: {
            ssid: '',
            connected: false
          }
        })
      }
    })
  },

  // 发送配网信息
  sendConfig() {
    if (!this.data.ssid || !this.data.password) {
      wx.showToast({
        title: '请输入WiFi信息',
        icon: 'none'
      })
      return
    }

    // 检查是否已连接到ESP32 AP
    wx.request({
      url: 'http://192.168.4.1:8080',
      method: 'GET',
      timeout: 3000,
      success: () => {
        // 可以访问ESP32，发送配网信息
        this.doSendConfig()
      },
      fail: () => {
        wx.showModal({
          title: '连接错误',
          content: '请确保已连接到ESP32的WiFi热点',
          showCancel: false
        })
      }
    })
  },

  // 执行配网请求
  doSendConfig() {
    wx.request({
      url: 'http://192.168.4.1:8080/config',
      method: 'POST',
      timeout: 10000,
      enableHttp2: true,
      header: {
        'content-type': 'application/json'
      },
      data: {
        ssid: this.data.ssid,
        password: this.data.password
      },
      success: () => {
        wx.showToast({
          title: '发送成功',
          icon: 'success'
        })
      },
      fail: (error) => {
        console.error('配网失败:', error)
        wx.showModal({
          title: '配网失败',
          content: '请检查WiFi信息是否正确，并确保ESP32在线',
          showCancel: false
        })
      }
    })
  },

  // 删除已连接WiFi
  deleteWifi() {
    wx.showModal({
      title: '确认删除',
      content: '确定要删除ESP32已连接的WiFi吗？',
      success: (res) => {
        if (res.confirm) {
          wx.request({
            url: 'http://192.168.4.1:8080/delete_wifi',
            method: 'POST',
            success: () => {
              wx.showToast({
                title: '删除成功',
                icon: 'success'
              })
            },
            fail: () => {
              wx.showModal({
                title: '删除失败',
                content: '请确保ESP32在线',
                showCancel: false
              })
            }
          })
        }
      }
    })
  }
})