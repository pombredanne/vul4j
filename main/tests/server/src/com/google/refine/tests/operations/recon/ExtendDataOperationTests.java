/*

Copyright 2010, Google Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
    * Neither the name of Google Inc. nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,           
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY           
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

package com.google.refine.tests.operations.recon;

import static org.mockito.Mockito.mock;

import java.io.IOException;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Properties;
import java.util.Set;

import org.slf4j.LoggerFactory;
import org.testng.Assert;
import org.testng.annotations.AfterMethod;
import org.testng.annotations.BeforeMethod;
import org.testng.annotations.BeforeTest;
import org.testng.annotations.Test;

import com.google.refine.browsing.Engine;
import com.google.refine.browsing.EngineConfig;
import com.google.refine.model.Cell;
import com.google.refine.model.ModelException;
import com.google.refine.model.Project;
import com.google.refine.model.Recon;
import com.google.refine.model.ReconCandidate;
import com.google.refine.model.Row;
import com.google.refine.model.recon.ReconciledDataExtensionJob;
import com.google.refine.model.recon.ReconciledDataExtensionJob.DataExtensionConfig;
import com.google.refine.operations.EngineDependentOperation;
import com.google.refine.operations.OperationRegistry;
import com.google.refine.operations.recon.ExtendDataOperation;
import com.google.refine.process.Process;
import com.google.refine.process.ProcessManager;
import com.google.refine.tests.RefineTest;
import com.google.refine.tests.util.TestUtils;
import com.google.refine.util.ParsingUtilities;


public class ExtendDataOperationTests extends RefineTest {

    static final String ENGINE_JSON_URLS = "{\"mode\":\"row-based\"}}";
    static final String RECON_SERVICE = "https://tools.wmflabs.org/openrefine-wikidata/en/api";
    static final String RECON_IDENTIFIER_SPACE = "http://www.wikidata.org/entity/";
    static final String RECON_SCHEMA_SPACE = "http://www.wikidata.org/prop/direct/";
    
    private String dataExtensionConfigJson = "{"
            + "    \"properties\":["
            + "        {\"name\":\"inception\",\"id\":\"P571\"},"
            + "        {\"name\":\"headquarters location\",\"id\":\"P159\"},"
            + "        {\"name\":\"coordinate location\",\"id\":\"P625\"}"
            + "     ]"
            + "}";
    
    private String operationJson = "{\"op\":\"core/extend-reconciled-data\","
            + "\"description\":\"Extend data at index 3 based on column organization_name\","
            + "\"engineConfig\":{\"mode\":\"row-based\",\"facets\":["
            + "    {\"selectNumeric\":true,\"expression\":\"cell.recon.best.score\",\"selectBlank\":false,\"selectNonNumeric\":true,\"selectError\":true,\"name\":\"organization_name: best candidate's score\",\"from\":13,\"to\":101,\"type\":\"range\",\"columnName\":\"organization_name\"},"
            + "    {\"selectNonTime\":true,\"expression\":\"grel:toDate(value)\",\"selectBlank\":true,\"selectError\":true,\"selectTime\":true,\"name\":\"start_year\",\"from\":410242968000,\"to\":1262309184000,\"type\":\"timerange\",\"columnName\":\"start_year\"}"
            + "]},"
            + "\"columnInsertIndex\":3,"
            + "\"baseColumnName\":\"organization_name\","
            + "\"endpoint\":\"https://tools.wmflabs.org/openrefine-wikidata/en/api\","
            + "\"identifierSpace\":\"http://www.wikidata.org/entity/\","
            + "\"schemaSpace\":\"http://www.wikidata.org/prop/direct/\","
            + "\"extension\":{"
            + "    \"properties\":["
            + "        {\"name\":\"inception\",\"id\":\"P571\"},"
            + "        {\"name\":\"headquarters location\",\"id\":\"P159\"},"
            + "        {\"name\":\"coordinate location\",\"id\":\"P625\"}"
            + "     ]"
            + "}}";
    
    private String processJson = ""
            + "    {\n" + 
            "       \"description\" : \"Extend data at index 3 based on column organization_name\",\n" + 
            "       \"id\" : %d,\n" + 
            "       \"immediate\" : false,\n" + 
            "       \"progress\" : 0,\n" + 
            "       \"status\" : \"pending\"\n" + 
            "     }";
    
    static public class ReconciledDataExtensionJobStub extends ReconciledDataExtensionJob {
        public ReconciledDataExtensionJobStub(DataExtensionConfig obj, String endpoint) {
            super(obj, endpoint);
        }

        public String formulateQueryStub(Set<String> ids, DataExtensionConfig node) throws IOException {
            StringWriter writer = new StringWriter();
            super.formulateQuery(ids, node, writer);
            return writer.toString();
        }
    }

    @Override
    @BeforeTest
    public void init() {
        logger = LoggerFactory.getLogger(this.getClass());
    }

    // dependencies
    Project project;
    Properties options;
    EngineConfig engine_config;
    Engine engine;

    @BeforeMethod
    public void SetUp() throws IOException, ModelException {
        OperationRegistry.registerOperation(getCoreModule(), "extend-reconciled-data", ExtendDataOperation.class);
        project = createProjectWithColumns("DataExtensionTests", "country");
        
        options = mock(Properties.class);
        engine = new Engine(project);
        engine_config = EngineConfig.reconstruct(ENGINE_JSON_URLS);
        engine.initializeFromConfig(engine_config);
        engine.setMode(Engine.Mode.RowBased);

               Row row = new Row(2);
        row.setCell(0, reconciledCell("Iran", "Q794"));
        project.rows.add(row);
        row = new Row(2);
        row.setCell(0, reconciledCell("Japan", "Q17"));
        project.rows.add(row);
        row = new Row(2);
        row.setCell(0, reconciledCell("Tajikistan", "Q863"));
        project.rows.add(row);
        row = new Row(2);
        row.setCell(0, reconciledCell("United States of America", "Q30"));
        project.rows.add(row);
    }
    
    @Test
    public void serializeExtendDataOperation() throws Exception {
        TestUtils.isSerializedTo(ParsingUtilities.mapper.readValue(operationJson, ExtendDataOperation.class), operationJson);
    }
    
    @Test
    public void serializeExtendDataProcess() throws Exception {
        Process p = ParsingUtilities.mapper.readValue(operationJson, ExtendDataOperation.class)
                .createProcess(project, new Properties());
        TestUtils.isSerializedTo(p, String.format(processJson, p.hashCode()));
    }
    
    @Test
    public void serializeDataExtensionConfig() throws IOException {
        TestUtils.isSerializedTo(DataExtensionConfig.reconstruct(dataExtensionConfigJson), dataExtensionConfigJson);
    }
    
    @Test
    public void testFormulateQuery() throws IOException {
        DataExtensionConfig config = DataExtensionConfig.reconstruct(dataExtensionConfigJson);
        Set<String> ids = Collections.singleton("Q2");
        String json = "{\"ids\":[\"Q2\"],\"properties\":[{\"id\":\"P571\"},{\"id\":\"P159\"},{\"id\":\"P625\"}]}";
        ReconciledDataExtensionJobStub stub = new ReconciledDataExtensionJobStub(config, "http://endpoint");
        TestUtils.assertEqualAsJson(json, stub.formulateQueryStub(ids, config));
    }
   

    @AfterMethod
    public void TearDown() {
        project = null;
        options = null;
        engine = null;
    }

    static public Cell reconciledCell(String name, String id) {
       ReconCandidate r = new ReconCandidate(id, name, new String[0], 100);
       List<ReconCandidate> candidates = new ArrayList<ReconCandidate>();
       candidates.add(r);
       Recon rec = new Recon(0, RECON_IDENTIFIER_SPACE, RECON_SCHEMA_SPACE);
       rec.service = RECON_SERVICE;
       rec.candidates = candidates;
       rec.match = r;
       return new Cell(name, rec);
    }
}
