package com.linux.permissionmanager.fragment;

import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.UnderlineSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.Switch;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.Fragment;

import com.linux.permissionmanager.BuildConfig;
import com.linux.permissionmanager.R;
import com.linux.permissionmanager.bridge.NativeBridge;
import com.linux.permissionmanager.utils.DialogUtils;

public class SettingsFragment extends Fragment {
    private Activity mActivity;
    private String mRootKey = "";

    private Button mBtnTestSkrootShellcodeChannel;
    private Button mBtnTestSkrootDefaultModule;
    private CheckBox mCkboxEnableSkrootLog;
    private Button mBtnShowSkrootLogs;
    private TextView mTvAboutVer;
    private TextView mTvLink;
    public SettingsFragment(Activity activity) {
        mActivity = activity;
    }
    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater,
                             @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_settings, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mBtnTestSkrootShellcodeChannel = view.findViewById(R.id.test_skroot_shellcode_channel_btn);
        mBtnTestSkrootDefaultModule = view.findViewById(R.id.test_skroot_default_module_btn);
        mCkboxEnableSkrootLog = view.findViewById(R.id.enable_skroot_log_ckbox);
        mBtnShowSkrootLogs = view.findViewById(R.id.show_skroot_logs_btn);
        mTvAboutVer = view.findViewById(R.id.about_ver_tv);
        mTvLink = view.findViewById(R.id.link_tv);
        initSettingsControl();
    }

    public void setRootKey(String rootKey) {
        mRootKey = rootKey;
    }

    private void initSettingsControl() {
        mBtnTestSkrootShellcodeChannel.setOnClickListener(
                (v) -> {
                    String tip = NativeBridge.testSkrootShellcodeChannel(mRootKey);
                    DialogUtils.showMsgDlg(mActivity, "执行结果", tip, null);
                }
        );
        mBtnTestSkrootDefaultModule.setOnClickListener((v) -> showSelectTestDefaultModuleDlg());
        mCkboxEnableSkrootLog.setChecked(NativeBridge.isEnableSkrootLog(mRootKey));
        mBtnShowSkrootLogs.setOnClickListener(v -> showSkrootLogsDlg());
        mCkboxEnableSkrootLog.setOnCheckedChangeListener(
                (v, isChecked) -> {
                    String tip = NativeBridge.setSkrootLogEnable(mRootKey, isChecked);
                    DialogUtils.showMsgDlg(mActivity, "执行结果", tip, null);
                }
        );
        initAboutText();
        initLink();
    }

    private void showSelectTestDefaultModuleDlg() {
        final String[] items = {"ROOT 权限模块", "su 重定向模块"};
        AlertDialog.Builder builder = new AlertDialog.Builder(mActivity);
        builder.setTitle("请选择一个选项");
        builder.setSingleChoiceItems(items, -1, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
                String defName = "";
                if(which == 0) defName = "RootBridge";
                else if(which == 1) defName = "SuRedirect";
                String tip = NativeBridge.testSkrootDefaultModule(mRootKey, defName);
                DialogUtils.showMsgDlg(mActivity, "执行结果", tip, null);
                if(tip.indexOf("OK") != -1 && !defName.equals("RootBridge")) {
                    showSkrootLogsDlg();;
                }
            }
        });
        builder.setNegativeButton("取消", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                dialog.dismiss();
            }
        });
        AlertDialog dialog = builder.create();
        dialog.show();
    }

    private void showSkrootLogsDlg() {
        String log = NativeBridge.readSkrootLogs(mRootKey);
        DialogUtils.showLogDialog(mActivity, log);
    }

    private void initAboutText() {
        StringBuffer sb = new StringBuffer();
        sb.append("内置核心版本：");
        sb.append(NativeBridge.getSdkSkrootEnvVersion());
        mTvAboutVer.setText(sb.toString());
    }

    private void initLink() {
        SpannableString ss = new SpannableString("https://github.com/abcz316/SKRoot-linuxKernelRoot");
        ss.setSpan(new UnderlineSpan(), 0, ss.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        mTvLink.setText(ss);
        mTvLink.setTextColor(Color.parseColor("#1E88E5"));
        mTvLink.setOnClickListener(v -> {
            Uri uri = Uri.parse("https://github.com/abcz316/SKRoot-linuxKernelRoot");
            Intent intent = new Intent(Intent.ACTION_VIEW, uri);
            v.getContext().startActivity(intent);
        });
    }

}
