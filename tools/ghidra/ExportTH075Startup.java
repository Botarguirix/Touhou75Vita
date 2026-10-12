// Export ten relevant functions from the known Japanese TH075 image.
// Pseudocode is analysis evidence, not verified or portable replacement code.
// @category TH075.Analysis
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.program.model.listing.Function;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.List;

public class ExportTH075Startup extends GhidraScript {
    private static final String SHA =
        "bd441e99075436e8dcad26f86ffcf5e6aac4f58b0ed3ee7442e4cb39d8e22c98";
    private static final long[] ADDRESSES = {
        0x401f50L,0x4027d0L,0x4028f0L,0x402970L,0x4029f0L,
        0x40c9a0L,0x425490L,0x425620L,0x432360L,0x432410L
    };
    @Override public void run() throws Exception {
        String[] args=getScriptArgs();
        if(args.length!=1)throw new IllegalArgumentException("One empty output directory required");
        if(currentProgram==null || !SHA.equalsIgnoreCase(currentProgram.getExecutableSHA256()) ||
            currentProgram.getImageBase().getOffset()!=0x400000L ||
            !currentProgram.getLanguageID().toString().startsWith("x86:LE:32:"))
            throw new IllegalStateException("Target hash/image base/language mismatch");
        Path executable=Paths.get(currentProgram.getExecutablePath());
        byte[] digest=MessageDigest.getInstance("SHA-256").digest(Files.readAllBytes(executable));
        StringBuilder actual=new StringBuilder();
        for(byte value:digest)actual.append(String.format("%02x",value&255));
        if(!SHA.equals(actual.toString()))throw new IllegalStateException("Executable file SHA256 mismatch");
        Path output=Paths.get(args[0]).toAbsolutePath().normalize();
        Files.createDirectories(output);
        try(java.util.stream.Stream<Path> files=Files.list(output)) {
            if(files.findAny().isPresent())throw new IllegalStateException("Output directory must be empty");
        }
        List<String> manifest=new ArrayList<>();
        manifest.add("sha256\t"+SHA);
        manifest.add("language\t"+currentProgram.getLanguageID());
        manifest.add("address\tstatus\tfunction\tfile_or_error");
        DecompInterface decompiler=new DecompInterface();
        int exported=0;
        try {
            if(!decompiler.openProgram(currentProgram))throw new IllegalStateException(decompiler.getLastMessage());
            for(long address:ADDRESSES) {
                monitor.checkCancelled();
                String hex=String.format("%08x",address);
                Function function=getFunctionAt(toAddr(address));
                if(function==null) {
                    manifest.add(hex+"\tmissing_function\t\tAuto-analysis did not establish this entry");continue;
                }
                DecompileResults result=decompiler.decompileFunction(function,60,monitor);
                if(!result.decompileCompleted() || result.getDecompiledFunction()==null) {
                    manifest.add(hex+"\tfailed\t"+function.getName()+"\t"+
                        String.valueOf(result.getErrorMessage()).replace('\t',' ').replace('\n',' '));continue;
                }
                String filename="function_"+hex+".c";
                String text="// SHA256: "+SHA+"\n// Entry: "+hex+
                    "\n// Unverified decompiler pseudocode. Keep outside the public code repository.\n"+
                    result.getDecompiledFunction().getC();
                Files.write(output.resolve(filename),text.getBytes(StandardCharsets.UTF_8),StandardOpenOption.CREATE_NEW);
                manifest.add(hex+"\texported\t"+function.getName()+"\t"+filename);++exported;
            }
        } finally {decompiler.dispose();}
        Files.write(output.resolve("manifest.tsv"),manifest,StandardCharsets.UTF_8,StandardOpenOption.CREATE_NEW);
        println("TH075_STARTUP_EXPORT "+exported+"/"+ADDRESSES.length+" -> "+output);
        if(exported!=ADDRESSES.length)throw new IllegalStateException("Incomplete export; inspect manifest.tsv");
    }
}
